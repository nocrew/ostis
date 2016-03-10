#include "common.h"
#include "mmu.h"
#include "floppy.h"
#include "floppy_msa.h"
#include "diag.h"

struct floppy_msa {
  char *filename;
  LONG raw_data_size;
  BYTE *raw_data;
};

#define FLOPPY_MSA(f, x) ((struct floppy_msa *)f->image_data)->x
#define SECSIZE 512

HANDLE_DIAGNOSTICS(floppy_msa);

static int read_sector(struct floppy *fl, int track, int side, int sector, LONG addr, int count)
{
  int pos,i,j;
  pos = track * (fl->sides+1) * fl->sectors * SECSIZE;
  pos += (sector-1) * SECSIZE;
  pos += side * fl->sectors * SECSIZE;

  for(i=0;i<count;i++) {
    /* Check if we're trying to read outside floppy data */
    if((pos+i*SECSIZE) >= (FLOPPY_MSA(fl, raw_data_size)-SECSIZE)) return FLOPPY_ERROR;
    for(j=0;j<SECSIZE;j++) {
      mmu_write_byte(addr+i*SECSIZE+j, FLOPPY_MSA(fl, raw_data)[pos+i*SECSIZE+j]);
    }
  }
  
  return FLOPPY_OK;
}

static int write_sector(struct floppy *fl, int track, int side, int sector, LONG addr, int count)
{
  return FLOPPY_ERROR;
}

static void load_file(struct floppy *fl, FILE *fp)
{
  BYTE msa_header[10];
  int starting_track, ending_track;
  int track,side,pos,tpos;
  int track_size;
  BYTE *track_data;
  int track_data_size;

  if(fread(msa_header, 10, 1, fp) != 1) {
    fclose(fp);
    return;
  }

  fl->sectors = (msa_header[2]<<8)|msa_header[3];
  fl->sides = (msa_header[4]<<8)|msa_header[5];
  starting_track = (msa_header[6]<<8)|msa_header[7];
  if(starting_track != 0) {
    ERROR("MSA files not starting with track 0 are unsupported.");
    return;
  }
  ending_track = (msa_header[8]<<8)|msa_header[9];
  fl->tracks = ending_track - starting_track + 1;

  track_size = fl->sectors * SECSIZE;
  
  track_data = (BYTE *)malloc(track_size);
  if(track_data == NULL) {
    ERROR("Unable to allocate track space");
    return;
  }

  FLOPPY_MSA(fl, raw_data) = floppy_allocate_memory();
  if(FLOPPY_MSA(fl, raw_data) == NULL) {
    ERROR("Unable to allocate floppy space");
    return;
  }

  FLOPPY_MSA(fl, raw_data_size) = (fl->sectors * SECSIZE) * fl->tracks * (fl->sides + 1);

  pos = 0;

  for(track = 0; track < fl->tracks; track++) {
    for(side = 0; side <= fl->sides; side++) {

      if(fread(track_data, 1, 2, fp) != 2) {
        ERROR("Unable to read track data");
        return;
      }
      track_data_size = (track_data[0]<<8)|track_data[1];
      if(fread(track_data, 1, track_data_size, fp) != track_data_size) {
        ERROR("Unable to read track data");
        return;
      }

      if(track_data_size != track_size) {
        for(tpos = 0; tpos < track_data_size; tpos++) {
          BYTE rep_byte;
          WORD rep_count;
          
          if(track_data[tpos] == 0xe5) {
            rep_byte = track_data[tpos+1];
            rep_count = (track_data[tpos+2]<<8)|track_data[tpos+3];
            if((pos + rep_count) > FLOPPY_MSA(fl, raw_data_size)) {
              ERROR("MSA RLE compression corrupt.");
              return;
            }
            memset(&FLOPPY_MSA(fl, raw_data)[pos], rep_byte, rep_count);
            pos += rep_count;
            tpos += 3;
          } else {
            FLOPPY_MSA(fl, raw_data)[pos] = track_data[tpos];
            pos++;
          }
        }
      } else {
        memcpy(&FLOPPY_MSA(fl, raw_data)[pos], track_data, track_data_size);
        pos += track_size;
      }
    }
  }

  fl->inserted = 1;
  return;
}


void floppy_msa_init(struct floppy *fl, char *name)
{
  FILE *fp;

  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(floppy_msa, "FMSA");
  
  fl->image_data = (void *)malloc(sizeof(struct floppy_msa));
  fl->image_data_size = sizeof(struct floppy_msa);
  FLOPPY_MSA(fl, filename) = name;

  fp = fopen(name, "rb");
  if(!fp) return;

  load_file(fl, fp);
  fl->read_sector = read_sector;
  fl->write_sector = write_sector;
}
