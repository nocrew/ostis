#include "common.h"
#include "mmu.h"
#include "floppy.h"

struct floppy {
  FILE *fp;
  int tracks;
  int sectors;
  int sides;
  int sel_trk;
  int sel_sec;
  int sel_side;
};

/* Currently only drive 0 (A:) supported */

struct floppy floppy[2];

void floppy_side(int side)
{
  if(!floppy[0].fp) return;
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
  if(!floppy[0].fp) return;
  floppy[0].sel_sec = sector;
  if(sector < 1)
    floppy[0].sel_sec = 1;
  if(sector > floppy[0].sectors)
    floppy[0].sel_sec = floppy[0].sectors;
}

int floppy_seek(int track)
{
  if(!floppy[0].fp) return FLOPPY_OK;
  floppy[0].sel_trk = track;
  if(track < 0)
    floppy[0].sel_trk = 0;
#if 1
  if(track >= floppy[0].tracks) {
    floppy[0].sel_trk = floppy[0].tracks-1;
  }
  if(track > floppy[0].tracks) return FLOPPY_ERROR;
#else
  if(track > 86) floppy[0].sel_trk = 86;
#endif

  return FLOPPY_OK;
}

int floppy_seek_rel(int off)
{
  if(!floppy[0].fp) return FLOPPY_ERROR;
  floppy[0].sel_trk += off;
  if(floppy[0].sel_trk < 0)
    floppy[0].sel_trk = 0;
#if 0
  if(floppy[0].sel_trk >= floppy[0].tracks)
    floppy[0].sel_trk = floppy[0].tracks-1;
#else
  if(floppy[0].sel_trk > 86) floppy[0].sel_trk = 86;
#endif

  return FLOPPY_OK;
}

int floppy_read_sector(LONG addr, int count)
{
  static BYTE sector[512];
  int pos,i,j;

  if(!floppy[0].fp) return FLOPPY_ERROR;
  if(floppy[0].sel_sec < 1) floppy[0].sel_sec = 1;

  pos = floppy[0].sel_trk*(floppy[0].sides+1)*floppy[0].sectors*512;
  pos += (floppy[0].sel_sec-1)*512;
  pos += floppy[0].sel_side*floppy[0].sectors*512;
#if 0
  printf("--------\n");
  printf("Read sector:\n");
  printf(" - Track:  %d\n", floppy[0].sel_trk);
  printf(" - Side:   %d\n", floppy[0].sel_side);
  printf(" - Sector: %d\n", floppy[0].sel_sec);
  printf(" = Pos:    %d\n", pos);
  printf("--------\n");
#endif
  fseek(floppy[0].fp, pos, SEEK_SET);
  for(i=0;i<count;i++) {
    if(fread(sector, 512, 1, floppy[0].fp) != 1) return FLOPPY_ERROR;
    for(j=0;j<512;j++) {
      mmu_write_byte(addr+i*512+j, sector[j]);
    }
  }
  return FLOPPY_OK;
}

void floppy_init(char *filename)
{
  static BYTE bootsector[512];

  if(floppy[0].fp)
    fclose(floppy[0].fp);
  floppy[0].fp = fopen(filename, "rb");
  if(!floppy[0].fp) return;

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

#if 0
  printf("---FLOPPY LAYOUT---\n");
  printf(" - Tracks:  %d\n", floppy[0].tracks);
  printf(" - Sides:   %d\n", floppy[0].sides+1);
  printf(" - Sectors: %d\n", floppy[0].sectors);
  printf("--------\n");
#endif
}
