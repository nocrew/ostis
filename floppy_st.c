#include "common.h"
#include "mmu.h"
#include "floppy.h"
#include "floppy_st.h"

struct floppy_st {
  char *filename;
  LONG raw_data_size;
  BYTE *raw_data;
};

#define FLOPPY_ST(f, x) ((struct floppy_st *)f->image_data)->x
#define SECSIZE 512

static void save_file(struct floppy *, long);

static int read_sector(struct floppy *fl, int track, int side, int sector, LONG addr, int count)
{
  int pos,i,j;
  pos = track * (fl->sides+1) * fl->sectors * SECSIZE;
  pos += (sector-1) * SECSIZE;
  pos += side * fl->sectors * SECSIZE;

  for(i=0;i<count;i++) {
    /* Check if we're trying to read outside floppy data */
    if((pos+i*SECSIZE) >= (FLOPPY_ST(fl, raw_data_size)-SECSIZE)) return FLOPPY_ERROR;
    for(j=0;j<SECSIZE;j++) {
      mmu_write_byte(addr+i*SECSIZE+j, FLOPPY_ST(fl, raw_data)[pos+i*SECSIZE+j]);
    }
  }
  
  return FLOPPY_OK;
}

static int write_sector(struct floppy *fl, int track, int side, int sector, LONG addr, int count)
{
  int pos,i,j;

  if(!fl->inserted) return FLOPPY_ERROR;
  if(!FLOPPY_ST(fl, filename)) return FLOPPY_ERROR;

  if(sector < 1) sector = 1;

  pos = track * (fl->sides+1) * fl->sectors * SECSIZE;
  pos += (sector-1) * SECSIZE;
  pos += side * fl->sectors * SECSIZE;

  for(i=0;i<count;i++) {
    if((pos+i*SECSIZE) >= (FLOPPY_ST(fl, raw_data_size)-SECSIZE)) return FLOPPY_ERROR;
    for(j=0;j<SECSIZE;j++) {
      FLOPPY_ST(fl, raw_data)[pos+i*SECSIZE+j] = mmu_read_byte(addr+i*SECSIZE+j);
    }
    save_file(fl, pos+i*SECSIZE);
  }
  return FLOPPY_OK;
}

static void save_file(struct floppy *fl, long offset)
{
  FILE *fp = fopen(FLOPPY_ST(fl, filename), "r+b");
  if(!fp) return;

  if(fseek(fp, offset, SEEK_SET) != 0) {
    printf("Error writing to floppy image\n");
  }

  if(fwrite(FLOPPY_ST(fl, raw_data) + offset, SECSIZE, 1, fp) != 1) {
    printf("Error writing to floppy image\n");
  }

  fclose(fp);
}

static void load_file(struct floppy *fl, FILE *fp)
{
  static BYTE bootsector[SECSIZE];

  fl->fp = fp;

  if(fread(bootsector, SECSIZE, 1, fl->fp) != 1) {
    fclose(fl->fp);
    fl->fp = NULL;
    return;
  }

  if((bootsector[11]|(bootsector[12]<<8)) != SECSIZE) {
    fclose(fl->fp);
    fl->fp = NULL;
    printf("Sector size != SECSIZE bytes (%d)\n", bootsector[11]|(bootsector[12]<<8));
    return;
  }

  fl->sectors = bootsector[24]|(bootsector[25]<<8);
  fl->sides = (bootsector[26]|(bootsector[27]<<8))-1;
  fl->tracks = ((bootsector[19]|(bootsector[20]<<8))/
		      (fl->sides+1)/fl->sectors);

  // Now we load the entire floppy into memory
  // Despite having fetched the track count, we'll allocate space for 86 tracks
  FLOPPY_ST(fl, raw_data) = floppy_allocate_memory();
  if(FLOPPY_ST(fl, raw_data) == NULL) {
    printf("Unable to allocate floppy space\n");
    return;
  }

  fseek(fl->fp, 0, SEEK_END);
  FLOPPY_ST(fl, raw_data_size) = ftell(fl->fp);
  rewind(fl->fp);
  if(fread(FLOPPY_ST(fl, raw_data), 1, FLOPPY_ST(fl, raw_data_size), fl->fp) != FLOPPY_ST(fl, raw_data_size)) {
    fclose(fl->fp);
    fl->fp = NULL;
    printf("Could not read entire file\n");
    return;
  }

  fclose(fl->fp);
  fl->fp = NULL;
  fl->inserted = 1;
}

void floppy_st_init(struct floppy *fl, char *name)
{
  FILE *fp;
  fl->image_data = (void *)malloc(sizeof(struct floppy_st));
  fl->image_data_size = sizeof(struct floppy_st);
  FLOPPY_ST(fl, filename) = name;

  fp = fopen(name, "rb");
  if(!fp) return;

  load_file(fl, fp);
  fl->read_sector = read_sector;
  fl->write_sector = write_sector;
}
