#include "common.h"
#include "mmu.h"
#include "floppy.h"
#include "floppy_stx.h"

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

static uint16_t stx_word(BYTE *x)
{
  return x[0] + (x[1] << 8);
}

static uint32_t stx_long(BYTE *x)
{
  return stx_word(x) + (stx_word(x + 2) << 16);
}

static void load_sector(int track, int side, int sector, int size, BYTE *data)
{
  BYTE *pos = raw_data;

  if (sector > 11)
    return;

  pos += track*(fl->sides+1)*fl->sectors*SECSIZE;
  pos += (sector-1)*SECSIZE;
  pos += side*fl->sectors*SECSIZE;

  memcpy(pos, data, size);
}

static void load_track(FILE *fp)
{
  int sectors, fuzzy, track_image, track_size;
  int sector_nr[70];
  int sector_size[70];
  int sector_side[70];
  int sector_track[70];
  int sector_offset[70];
  BYTE header[16];
  BYTE data[50000];
  BYTE *pos;
  int i;

  if(fread(header, 16, 1, fp) != 1) {
    printf("ERROR\n");
    fclose(fp);
    return;
  }

  track_size = stx_long(header);
  fuzzy = stx_long(header + 4);
  sectors = stx_word(header + 8);
  track_image = header[10] & 0xC0;

  if(sectors == 0)
    return;

  if(fread(data, track_size - 16, 1, fp) != 1) {
    printf("ERROR\n");
    fclose(fp);
    return;
  }

  pos = data;

  if(header[10] & 1) {
    for(i = 0; i < sectors; i++) {
      sector_offset[i] = stx_long(pos);
      sector_track[i] = pos[8];
      sector_side[i] = pos[9];
      sector_nr[i] = pos[10];
      sector_size[i] = (128 << (pos[11] & 3));
      pos += 16;
    }
  } else {
    for(i = 0; i < sectors; i++) {
      sector_offset[i] = i * SECSIZE;
      sector_track[i] = (header[14] & 0x7f);
      sector_side[i] = ((header[14] & 0x80) >> 7);
      sector_nr[i] = i + 1;
      sector_size[i] = SECSIZE;
    }
  }

  pos += fuzzy;

  if(track_image & 0x40) {
    int off = 0, ts;
    BYTE *p = pos;
    if (track_image & 0x80) {
      off = stx_word(p);
      p += 2;
    }
    ts = stx_word(p);
    printf("Track image offset %d size %d\n", off, ts);
  }

  for(i = 0; i < sectors; i++) {
    load_sector(sector_track[i], sector_side[i], sector_nr[i], sector_size[i],
		pos + sector_offset[i]);
  }
}

static void load_file(FILE *fp)
{
  BYTE header[16];
  int tracks;
  int i;

  if(fread(header, 16, 1, fp) != 1) {
    printf("ERROR\n");
    fclose(fp);
    return;
  }

  tracks = header[10];

  fl->sectors = 11;
  fl->sides = 1;
  fl->tracks = tracks;
  raw_data_size = (fl->sectors * SECSIZE) * fl->tracks * (fl->sides + 1);

  raw_data = floppy_allocate_memory();
  if(raw_data == NULL) {
    printf("Unable to allocate floppy space\n");
    return;
  }

  for(i = 0; i < tracks; i++) {
    load_track(fp);
  }

  fl->fp = NULL;
  fl->inserted = 1;

  fclose(fp);
  return;
}

void floppy_stx_init(struct floppy *flop, char *name)
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
