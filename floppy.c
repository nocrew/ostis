#include "common.h"
#include "mmu.h"
#include "floppy.h"
#include "floppy_st.h"
#include "floppy_msa.h"
#include "floppy_stx.h"
#include "diag.h"

/* Currently only drive 0 (A:) supported */

struct floppy floppy[2];
BYTE *floppy_raw_data;
LONG floppy_raw_data_size;

HANDLE_DIAGNOSTICS(floppy);

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
  return floppy[0].read_sector(floppy[0].sel_trk, floppy[0].sel_side, floppy[0].sel_sec, addr, count);
}

int floppy_write_sector(LONG addr, int count)
{
  return floppy[0].write_sector(floppy[0].sel_trk, floppy[0].sel_side, floppy[0].sel_sec, addr, count);
}

BYTE *floppy_allocate_memory()
{
  return (BYTE *)malloc(86 * (floppy[0].sides+1) * floppy[0].sectors * 512);
}

static int dummy_read_sector(int track, int side, int sector, LONG addr, int count)
{
  return FLOPPY_ERROR;
}

static int dummy_write_sector(int track, int side, int sector, LONG addr, int count)
{
  return FLOPPY_ERROR;
}

void floppy_init(char *filename)
{
  BYTE header[512];

  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(floppy, "FLOP");
  
  floppy[0].inserted = 0;
  floppy[0].read_sector = dummy_read_sector;
  floppy[0].write_sector = dummy_write_sector;
  
  FILE *fp = fopen(filename, "rb");
  if(!fp) return;

  if(fread(header, 512, 1, fp) != 1) {
    fclose(fp);
    return;
  }

  fclose(fp);
  
  // If header starts with 0x0e0f we treat this as an MSA file,
  // otherwise it's considered to be a raw image.
  if(header[0] == 0x0e && header[1] == 0x0f) {
    floppy_msa_init(&floppy[0], filename);
  } else if(header[0] == 0x52 && header[1] == 0x53 &&
	    header[2] == 0x59 && header[3] == 0x00) {
    floppy_stx_init(&floppy[0], filename);
  } else { 
    floppy_st_init(&floppy[0], filename);
  }
}
