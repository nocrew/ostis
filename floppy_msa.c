#include "common.h"
#include "mmu.h"
#include "floppy.h"
#include "floppy_msa.h"

static struct floppy *fl;
static char *filename = NULL;
static BYTE *raw_data;
static LONG raw_data_size;

#define SECSIZE 512

static int read_sector(int track, int side, int sector, LONG addr, int count)
{
  int pos,i,j;
  pos = track * (fl->sides+1) * fl->sectors * SECSIZE;
  pos += (sector-1) * SECSIZE;
  pos += side * fl->sectors * SECSIZE;

  for(i=0;i<count;i++) {
    /* Check if we're trying to read outside floppy data */
    if((pos+i*SECSIZE) >= (raw_data_size-SECSIZE)) return FLOPPY_ERROR;
    for(j=0;j<SECSIZE;j++) {
      mmu_write_byte(addr+i*SECSIZE+j, raw_data[pos+i*SECSIZE+j]);
    }
  }
  
  return FLOPPY_OK;
}

static int write_sector(int track, int side, int sector, LONG addr, int count)
{
  return FLOPPY_ERROR;
}

static void load_file(FILE *fp)
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
    printf("Error: MSA files not starting with track 0 are unsupported.\n");
    return;
  }
  ending_track = (msa_header[8]<<8)|msa_header[9];
  fl->tracks = ending_track - starting_track + 1;

  track_size = fl->sectors * SECSIZE;
  
  track_data = (BYTE *)malloc(track_size);
  if(track_data == NULL) {
    printf("Error: Unable to allocate track space\n");
    return;
  }

  raw_data = floppy_allocate_memory();
  if(raw_data == NULL) {
    printf("Unable to allocate floppy space\n");
    return;
  }

  raw_data_size = (fl->sectors * SECSIZE) * fl->tracks * (fl->sides + 1);

  pos = 0;

  for(track = 0; track < fl->tracks; track++) {
    for(side = 0; side <= fl->sides; side++) {

      if(fread(track_data, 1, 2, fp) != 2) {
        printf("Error: Unable to read track data\n");
        return;
      }
      track_data_size = (track_data[0]<<8)|track_data[1];
      if(fread(track_data, 1, track_data_size, fp) != track_data_size) {
        printf("Error: Unable to read track data\n");
        return;
      }

      if(track_data_size != track_size) {
        for(tpos = 0; tpos < track_data_size; tpos++) {
          BYTE rep_byte;
          WORD rep_count;
          
          if(track_data[tpos] == 0xe5) {
            rep_byte = track_data[tpos+1];
            rep_count = (track_data[tpos+2]<<8)|track_data[tpos+3];
            if((pos + rep_count) > raw_data_size) {
              printf("Error: MSA RLE compression corrupt.\n");
              return;
            }
            memset(&raw_data[pos], rep_byte, rep_count);
            pos += rep_count;
            tpos += 3;
          } else {
            raw_data[pos] = track_data[tpos];
            pos++;
          }
        }
      } else {
        memcpy(&raw_data[pos], track_data, track_data_size);
        pos += track_size;
      }
    }
  }

  fl->inserted = 1;
  return;
}


void floppy_msa_init(struct floppy *flop, char *name)
{
  FILE *fp;
  fl = flop;
  filename = name;

  fp = fopen(filename, "rb");
  if(!fp) return;

  load_file(fp);
  fl->read_sector = read_sector;
  fl->write_sector = write_sector;
}
