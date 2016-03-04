#include "common.h"
#include "mmu.h"
#include "floppy.h"
#include "floppy_stx.h"
#include "diag.h"

/* Currently only drive 0 (A:) supported */

struct floppy floppy[2];
BYTE *floppy_raw_data;
LONG floppy_raw_data_size;
static const char *floppy_filename = NULL;

HANDLE_DIAGNOSTICS(floppy);

static void floppy_store_raw(long offset);

void floppy_side(int side)
{
  if(!floppy[0].inserted) return;
  TRACE("floppy_side: Side: %d", side + 1);
  if(side == 0)
    floppy[0].sel_side = 1;
  else
    floppy[0].sel_side = 0;
}

void floppy_active(int dmask)
{
  
}

void floppy_sector(int sector)
{
  if(!floppy[0].inserted) return;
  TRACE("floppy_sector: Track %d - Sector %d", floppy[0].sel_trk, sector);
  floppy[0].sel_sec = sector;
  if(sector < 1)
    floppy[0].sel_sec = 1;
  if(sector > floppy[0].sectors)
    floppy[0].sel_sec = floppy[0].sectors;
}

int floppy_seek(int track)
{
  if(!floppy[0].inserted) return FLOPPY_OK;
  TRACE("floppy_seek: Track %d", track);
  floppy[0].sel_trk = track;
  if(track < 0)
    floppy[0].sel_trk = 0;

  if(track >= floppy[0].tracks) {
    floppy[0].sel_trk = floppy[0].tracks-1;
  }
  if(track > floppy[0].tracks) return FLOPPY_ERROR;

  return FLOPPY_OK;
}

int floppy_seek_rel(int off)
{ 
  if(!floppy[0].inserted) return FLOPPY_ERROR;
  TRACE("floppy_seek_rel: Track %d", floppy[0].sel_trk + off);
  floppy[0].sel_trk += off;
  if(floppy[0].sel_trk < 0)
    floppy[0].sel_trk = 0;

  if(floppy[0].sel_trk > 86) floppy[0].sel_trk = 86;

  return FLOPPY_OK;
}

int floppy_read_sector(LONG addr, int count)
{
  int pos,i,j;

  if(!floppy[0].inserted) return FLOPPY_ERROR;
  if(floppy[0].sel_sec < 1) floppy[0].sel_sec = 1;

  pos = floppy[0].sel_trk*(floppy[0].sides+1)*floppy[0].sectors*512;
  pos += (floppy[0].sel_sec-1)*512;
  pos += floppy[0].sel_side*floppy[0].sectors*512;

  DEBUG("floppy_read_sector: Track %*d - Sector %*d - Count: %*d", 2, floppy[0].sel_trk, 2, floppy[0].sel_sec, 2, count);

  for(i=0;i<count;i++) {
    if((pos+i*512) >= (floppy_raw_data_size-512)) return FLOPPY_ERROR;
    for(j=0;j<512;j++) {
      mmu_write_byte(addr+i*512+j, floppy_raw_data[pos+i*512+j]);
    }
  }
  return FLOPPY_OK;
}

int floppy_write_sector(LONG addr, int count)
{
  int pos,i,j;

  if(!floppy[0].inserted) return FLOPPY_ERROR;
  if(!floppy_filename) return FLOPPY_ERROR;

  if(floppy[0].sel_sec < 1) floppy[0].sel_sec = 1;

  pos = floppy[0].sel_trk*(floppy[0].sides+1)*floppy[0].sectors*512;
  pos += (floppy[0].sel_sec-1)*512;
  pos += floppy[0].sel_side*floppy[0].sectors*512;

  DEBUG("floppy_write_sector: Track %*d - Sector %*d - Count: %*d", 2, floppy[0].sel_trk, 2, floppy[0].sel_sec, 2, count);

  for(i=0;i<count;i++) {
    if((pos+i*512) >= (floppy_raw_data_size-512)) return FLOPPY_ERROR;
    for(j=0;j<512;j++) {
      floppy_raw_data[pos+i*512+j] = mmu_read_byte(addr+i*512+j);
    }
    floppy_store_raw(pos+i*512);
  }
  return FLOPPY_OK;
}

BYTE *floppy_allocate_memory()
{
  return (BYTE *)malloc(86 * (floppy[0].sides+1) * floppy[0].sectors * 512);
}

static void floppy_store_raw(long offset)
{
  FILE *fp = fopen(floppy_filename, "r+b");
  if(!fp) return;

  if(fseek(fp, offset, SEEK_SET) != 0) {
    printf("Error writing to floppy image\n");
  }

  if(fwrite(floppy_raw_data + offset, 512, 1, fp) != 1) {
    printf("Error writing to floppy image\n");
  }

  fclose(fp);
}

void floppy_load_raw(FILE *fp)
{
  static BYTE bootsector[512];

  floppy[0].fp = fp;

  if(fread(bootsector, 512, 1, floppy[0].fp) != 1) {
    fclose(floppy[0].fp);
    floppy[0].fp = NULL;
    return;
  }

  if((bootsector[11]|(bootsector[12]<<8)) != 512) {
    fclose(floppy[0].fp);
    floppy[0].fp = NULL;
    printf("Sector size != 512 bytes (%d)\n", bootsector[11]|(bootsector[12]<<8));
    return;
  }

  floppy[0].sectors = bootsector[24]|(bootsector[25]<<8);
  floppy[0].sides = (bootsector[26]|(bootsector[27]<<8))-1;
  floppy[0].tracks = ((bootsector[19]|(bootsector[20]<<8))/
		      (floppy[0].sides+1)/floppy[0].sectors);

  // Now we load the entire floppy into memory
  // Despite having fetched the track count, we'll allocate space for 86 tracks
  floppy_raw_data = floppy_allocate_memory();
  if(floppy_raw_data == NULL) {
    printf("Unable to allocate floppy space\n");
    return;
  }

  fseek(floppy[0].fp, 0, SEEK_END);
  floppy_raw_data_size = ftell(floppy[0].fp);
  rewind(floppy[0].fp);
  if(fread(floppy_raw_data, 1, floppy_raw_data_size, floppy[0].fp) != floppy_raw_data_size) {
    fclose(floppy[0].fp);
    floppy[0].fp = NULL;
    printf("Could not read entire file\n");
    return;
  }

  fclose(floppy[0].fp);
  floppy[0].fp = NULL;
  floppy[0].inserted = 1;
}

void floppy_load_msa(FILE *fp)
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

  floppy[0].sectors = (msa_header[2]<<8)|msa_header[3];
  floppy[0].sides = (msa_header[4]<<8)|msa_header[5];
  starting_track = (msa_header[6]<<8)|msa_header[7];
  if(starting_track != 0) {
    printf("Error: MSA files not starting with track 0 are unsupported.\n");
    return;
  }
  ending_track = (msa_header[8]<<8)|msa_header[9];
  floppy[0].tracks = ending_track - starting_track + 1;

  track_size = floppy[0].sectors * 512;
  
  track_data = (BYTE *)malloc(track_size);
  if(track_data == NULL) {
    printf("Error: Unable to allocate track space\n");
    return;
  }

  floppy_raw_data = floppy_allocate_memory();
  if(floppy_raw_data == NULL) {
    printf("Unable to allocate floppy space\n");
    return;
  }

  floppy_raw_data_size = (floppy[0].sectors * 512) * floppy[0].tracks * (floppy[0].sides + 1);

  pos = 0;

  for(track = 0; track < floppy[0].tracks; track++) {
    for(side = 0; side <= floppy[0].sides; side++) {

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
            if((pos + rep_count) > floppy_raw_data_size) {
              printf("Error: MSA RLE compression corrupt.\n");
              return;
            }
            memset(&floppy_raw_data[pos], rep_byte, rep_count);
            pos += rep_count;
            tpos += 3;
          } else {
            floppy_raw_data[pos] = track_data[tpos];
            pos++;
          }
        }
      } else {
        memcpy(&floppy_raw_data[pos], track_data, track_data_size);
        pos += track_size;
      }
    }
  }

  floppy[0].inserted = 1;
  return;
}

void floppy_init(char *filename)
{
  BYTE header[512];

  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(floppy, "FLOP");
  
  floppy[0].inserted = 0;
  
  FILE *fp = fopen(filename, "rb");
  if(!fp) return;

  if(fread(header, 512, 1, fp) != 1) {
    fclose(fp);
    return;
  }

  rewind(fp);

  // If header starts with 0x0e0f we treat this as an MSA file,
  // otherwise it's considered to be a raw image.
  if(header[0] == 0x0e && header[1] == 0x0f) {
    floppy_load_msa(fp);
  } else if(header[0] == 0x52 && header[1] == 0x53 &&
	    header[2] == 0x59 && header[3] == 0x00) {
    floppy_load_stx(fp);
  } else {
    floppy_filename = filename;
    floppy_load_raw(fp);
  }
}
