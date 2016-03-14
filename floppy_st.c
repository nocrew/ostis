#include "common.h"
#include "mmu.h"
#include "floppy.h"
#include "floppy_st.h"
#include "diag.h"

struct floppy_st {
  LONG raw_data_size;
  BYTE *raw_data;
};

#define SECSIZE 512

HANDLE_DIAGNOSTICS(floppy_st);

static void save_file(struct floppy *, long);

static int read_sector(struct floppy *fl, int track, int side, int sector, LONG addr, int count)
{
  int pos,i,j;
  pos = track * (fl->sides+1) * fl->sectors * SECSIZE;
  pos += (sector-1) * SECSIZE;
  pos += side * fl->sectors * SECSIZE;
  struct floppy_st *image = fl->image_data;

  for(i=0;i<count;i++) {
    /* Check if we're trying to read outside floppy data */
    if((pos+i*SECSIZE) >= (image->raw_data_size-SECSIZE)) return FLOPPY_ERROR;
    for(j=0;j<SECSIZE;j++) {
      mmu_write_byte(addr+i*SECSIZE+j, image->raw_data[pos+i*SECSIZE+j]);
    }
  }
  
  return FLOPPY_OK;
}

static int write_sector(struct floppy *fl, int track, int side, int sector, LONG addr, int count)
{
  struct floppy_st *image = fl->image_data;
  int pos,i,j;

  if(!fl->inserted) return FLOPPY_ERROR;
  if(!fl->filename) return FLOPPY_ERROR;

  if(sector < 1) sector = 1;

  pos = track * (fl->sides+1) * fl->sectors * SECSIZE;
  pos += (sector-1) * SECSIZE;
  pos += side * fl->sectors * SECSIZE;

  for(i=0;i<count;i++) {
    if((pos+i*SECSIZE) >= (image->raw_data_size-SECSIZE)) return FLOPPY_ERROR;
    for(j=0;j<SECSIZE;j++) {
      image->raw_data[pos+i*SECSIZE+j] = mmu_read_byte(addr+i*SECSIZE+j);
    }
    save_file(fl, pos+i*SECSIZE);
  }
  return FLOPPY_OK;
}

static void save_file(struct floppy *fl, long offset)
{
  struct floppy_st *image = fl->image_data;
  FILE *fp = fopen(fl->filename, "r+b");
  if(!fp) return;

  if(fseek(fp, offset, SEEK_SET) != 0) {
    ERROR("Error writing to floppy image");
  }

  if(fwrite(image->raw_data + offset, SECSIZE, 1, fp) != 1) {
    ERROR("Error writing to floppy image");
  }

  fclose(fp);
}

static int guess_format(struct floppy *fl) {
  struct floppy_st *image = fl->image_data;

  if((image->raw_data_size % (SECSIZE * 9)) == 0)
    fl->sectors = 9;
  else if((image->raw_data_size % (SECSIZE * 10)) == 0)
    fl->sectors = 10;
  else if((image->raw_data_size % (SECSIZE * 11)) == 0)
    fl->sectors = 11;
  else
    return -1;

  fl->tracks = image->raw_data_size / SECSIZE / fl->sectors;
  if(fl->tracks >= 80 && fl->tracks <= 86)
    fl->sides = 0;
  else if(fl->tracks >= 160 && fl->tracks <= 2*86 && (fl->tracks % 2) == 0) {
    fl->sides = 1;
    fl->tracks >>= 1;
  } else
    return -1;

  return 0;
}

static void load_file(struct floppy *fl, FILE *fp)
{
  struct floppy_st *image = fl->image_data;
  static BYTE bootsector[SECSIZE];

  fl->fp = fp;

  if(fread(bootsector, SECSIZE, 1, fl->fp) != 1) {
    fclose(fl->fp);
    fl->fp = NULL;
    return;
  }

  fl->sectors = bootsector[24]|(bootsector[25]<<8);
  fl->sides = (bootsector[26]|(bootsector[27]<<8))-1;
  if(fl->sides != -1)
    fl->tracks = ((bootsector[19]|(bootsector[20]<<8))/
		  (fl->sides+1)/fl->sectors);

  fseek(fl->fp, 0, SEEK_END);
  image->raw_data_size = ftell(fl->fp);
  rewind(fl->fp);

  if((bootsector[11]|(bootsector[12]<<8)) != SECSIZE ||
     fl->sectors < 9 || fl->sectors > 11 ||
     fl->sides < 0 || fl->sides > 1 ||
     fl->tracks < 80 || fl->tracks > 86) {
    INFO("Bad boot sector in .st image file");
    if (guess_format(fl) == -1) {
      fclose(fl->fp);
      fl->fp = NULL;
      ERROR("Sector size != SECSIZE bytes (%d)", bootsector[11]|(bootsector[12]<<8));
      return;
    }
  }

  // Now we load the entire floppy into memory
  // Despite having fetched the track count, we'll allocate space for 86 tracks
  image->raw_data = floppy_allocate_memory();
  if(image->raw_data == NULL) {
    ERROR("Unable to allocate floppy space");
    return;
  }

  if(fread(image->raw_data, 1, image->raw_data_size, fl->fp) != image->raw_data_size) {
    fclose(fl->fp);
    fl->fp = NULL;
    ERROR("Could not read entire file");
    return;
  }

  fclose(fl->fp);
  fl->fp = NULL;
  fl->inserted = 1;
}

void floppy_st_init(struct floppy *fl)
{
  FILE *fp;

  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(floppy_st, "FLST");
  
  fl->image_data = malloc(sizeof(struct floppy_st));

  fp = fopen(fl->filename, "rb");
  if(!fp) return;

  load_file(fl, fp);
  fl->read_sector = read_sector;
  fl->write_sector = write_sector;
}
