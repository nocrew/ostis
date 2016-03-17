#include "common.h"
#include "mmu.h"
#include "floppy.h"
#include "floppy_st.h"
#include "floppy_msa.h"
#include "floppy_stx.h"
#include "diag.h"

/* Currently only drive 0 (A:) supported */

#define FLOPPY_A 0
#define FLOPPY_B 1
#define FLOPPY_OFF 2

struct floppy floppy[3];
static int active_device = FLOPPY_OFF;
BYTE *floppy_raw_data;
LONG floppy_raw_data_size;

HANDLE_DIAGNOSTICS(floppy);

/* Floppy side is completely separate from floppy device selection,
 * so we'll just write the same selected side to both drives. */
void floppy_side(int side)
{
  int sel_side = side == 0 ? 1 : 0;
  
  floppy[FLOPPY_A].sel_side = sel_side;
  floppy[FLOPPY_B].sel_side = sel_side;
}

void floppy_active(int dmask)
{
  if(dmask == 3) {
    TRACE("Active device: %d => %d", active_device, 2);
    active_device = FLOPPY_OFF;
  } else if(dmask == 1) {
    TRACE("Active device: %d => %d", active_device, 1);
    active_device = FLOPPY_B;
  } else if(dmask == 2) {
    TRACE("Active device: %d => %d", active_device, 0);
    active_device = FLOPPY_A;
  } else {
    /* If both drives are active at once, we will treat them as if only A is */
    active_device = FLOPPY_A;
  }
}

void floppy_sector(int sector)
{
  if(!floppy[active_device].inserted) return;
  TRACE("floppy_sector: Track %d - Sector %d", floppy[active_device].sel_trk, sector);
  floppy[active_device].sel_sec = sector;
}

int floppy_current_track()
{
  if(!floppy[active_device].inserted) return 0;
  return floppy[active_device].sel_trk;
}

int floppy_seek(int track)
{
  TRACE("floppy_seek: Dr: %d  Track %d", active_device, track);
  if(!floppy[active_device].inserted) return FLOPPY_ERROR;
  TRACE("floppy_seek: Disk inserted");
  floppy[active_device].sel_trk = track;
  if(track < 0)
    floppy[active_device].sel_trk = 0;

  if(track >= floppy[active_device].tracks) {
    floppy[active_device].sel_trk = floppy[active_device].tracks-1;
  }
  if(track > floppy[active_device].tracks) return FLOPPY_ERROR;

  return FLOPPY_OK;
}

int floppy_seek_rel(int off)
{ 
  TRACE("floppy_seek_rel: Track %d", floppy[active_device].sel_trk + off);
  if(!floppy[active_device].inserted) return FLOPPY_ERROR;
  floppy[active_device].sel_trk += off;
  if(floppy[active_device].sel_trk < 0)
    floppy[active_device].sel_trk = 0;

  if(floppy[active_device].sel_trk > 86) floppy[active_device].sel_trk = 86;

  return FLOPPY_OK;
}

int floppy_read_sector(LONG addr, int count)
{
  LONG off;

  off = floppy[active_device].sel_trk * (floppy[active_device].sides+1) * floppy[active_device].sectors * 512;
  off += (floppy[active_device].sel_sec-1) * 512;
  off += floppy[active_device].sel_side * floppy[active_device].sectors * 512;
  
  TRACE("ReadSector: Dr: %d  T: %d  Sd: %d  S: %d  Off: %d  A: %06x  C: %d",
        active_device, 
        floppy[active_device].sel_trk,
        floppy[active_device].sel_side,
        floppy[active_device].sel_sec,
        off,
        addr, count);
  return floppy[active_device].read_sector(&floppy[active_device], floppy[active_device].sel_trk, floppy[active_device].sel_side, floppy[active_device].sel_sec, addr, count);
}

int floppy_write_sector(LONG addr, int count)
{
  return floppy[active_device].write_sector(&floppy[active_device], floppy[active_device].sel_trk, floppy[active_device].sel_side, floppy[active_device].sel_sec, addr, count);
}

int floppy_read_track(LONG addr, int dma_count)
{
  return floppy[active_device].read_track(&floppy[active_device], floppy[active_device].sel_trk, floppy[active_device].sel_side, addr, dma_count);
}

int floppy_write_track(LONG addr, int dma_count)
{
  return floppy[active_device].write_track(&floppy[active_device], floppy[active_device].sel_trk, floppy[active_device].sel_side, addr, dma_count);
}

int floppy_read_address(LONG addr)
{
  mmu_write_byte(addr, floppy[active_device].sel_trk);
  mmu_write_byte(addr+1, floppy[active_device].sel_side);
  mmu_write_byte(addr+2, floppy[active_device].sel_sec);
  mmu_write_byte(addr+3, 2); /* 512 byte for now */
  mmu_write_byte(addr+4, 0);
  mmu_write_byte(addr+5, 0);
  return 0;
}

BYTE *floppy_allocate_memory()
{
  return xmalloc(86 * 2 * 7000); /* Should be large enough to cover all floppy data */
}

static int dummy_read_sector(struct floppy *fl, int track, int side, int sector, LONG addr, int count)
{
  return FLOPPY_ERROR;
}

static int dummy_write_sector(struct floppy *fl, int track, int side, int sector, LONG addr, int count)
{
  return FLOPPY_ERROR;
}

static int dummy_read_track(struct floppy *fl, int track, int side, LONG addr, int dma_count)
{
  DEBUG("DUMMY: Read track not implemented");
  TRACE("DUMMY: ReadTrk: T: %d  Sd: %d  A: %06x  C: %d", track, side, addr, dma_count);
  return FLOPPY_ERROR;
}

static int dummy_write_track(struct floppy *fl, int track, int side, LONG addr, int dma_count)
{
  DEBUG("DUMMY: Write track not implemented");
  TRACE("DUMMY: WriteTrk: T: %d  Sd: %d  A: %06x  C: %d", track, side, addr, dma_count);
  return FLOPPY_ERROR;
}

static void floppy_set_drive_default(int drive_id)
{
  floppy[drive_id].inserted = 0;
  floppy[drive_id].read_sector = dummy_read_sector;
  floppy[drive_id].write_sector = dummy_write_sector;
  floppy[drive_id].read_track = dummy_read_track;
  floppy[drive_id].write_track = dummy_write_track;
}

void load_floppy(int device, char *filename)
{
  BYTE header[512];

  FILE *fp = fopen(filename, "rb");
  if(!fp) return;

  if(fread(header, 512, 1, fp) != 1) {
    fclose(fp);
    return;
  }

  fclose(fp);
  
  floppy[device].filename = filename;

  // If header starts with 0x0e0f we treat this as an MSA file,
  // if it starts with 0x52535900 it's an STX image,
  // otherwise it's considered to be a raw image.
  if(header[0] == 0x0e && header[1] == 0x0f) {
    floppy_msa_init(&floppy[device]);
  } else if(header[0] == 0x52 && header[1] == 0x53 &&
	    header[2] == 0x59 && header[3] == 0x00) {
    floppy_stx_init(&floppy[device]);
  } else { 
    floppy_st_init(&floppy[device]);
  }
}
void floppy_init(char *filename, char *filename2)
{
  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(floppy, "FLOP");

  /* Dummy floppy device for drive A and B, and "No drive" so that there are dummy functions
   * when the floppy is not mounted */
  floppy_set_drive_default(FLOPPY_OFF);
  floppy_set_drive_default(FLOPPY_A);
  floppy_set_drive_default(FLOPPY_B);

  if(filename) {
    load_floppy(FLOPPY_A, filename);
  }

  if(filename2) {
    load_floppy(FLOPPY_B, filename2);
  }
}
