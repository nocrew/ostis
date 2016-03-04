#include "common.h"
#include "floppy.h"
#include "floppy_stx.h"

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
  BYTE *pos = floppy_raw_data;

  if (sector > 11)
    return;

  pos += track*(floppy[0].sides+1)*floppy[0].sectors*512;
  pos += (sector-1)*512;
  pos += side*floppy[0].sectors*512;

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
      sector_offset[i] = i * 512;
      sector_track[i] = (header[14] & 0x7f);
      sector_side[i] = ((header[14] & 0x80) >> 7);
      sector_nr[i] = i + 1;
      sector_size[i] = 512;
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

void floppy_load_stx(FILE *fp)
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

  floppy[0].sectors = 11;
  floppy[0].sides = 1;
  floppy[0].tracks = tracks;
  floppy_raw_data_size = (floppy[0].sectors * 512) * floppy[0].tracks * (floppy[0].sides + 1);

  floppy_raw_data = floppy_allocate_memory();
  if(floppy_raw_data == NULL) {
    printf("Unable to allocate floppy space\n");
    return;
  }

  for(i = 0; i < tracks; i++) {
    load_track(fp);
  }

  floppy[0].fp = NULL;
  floppy[0].inserted = 1;

  fclose(fp);
  return;
}
