#include "common.h"
#include "mmu.h"
#include "floppy.h"
#include "floppy_stx.h"
#include "diag.h"

#define MAXSIDES 2
#define MAXTRACKS 86

struct sector {
  int nr;
  int size;
  int offset;
  int is_fuzzy;
  BYTE *data;
  BYTE *fuzzy_mask;
};

struct track {
  int nr;
  int side;
  int size;
  int fuzzy_size;
  int sector_count;
  struct sector *sectors;
};

struct floppy_stx {
  char *filename;
  struct track tracks[MAXTRACKS*2]; /* Both sides */
};
  
#define FLOPPY_STX(f, x) ((struct floppy_stx *)f->image_data)->x
#define SECSIZE 512

HANDLE_DIAGNOSTICS(floppy_stx);

static int read_sector(struct floppy *fl, int track, int side, int sector, LONG addr, int count)
{
  struct track *tracks;
  int i,j,dst_pos;
  struct sector *track_sectors;
  BYTE *fuzzy_mask;
  int track_side_num;
  int sector_count;
  int sec_num, sec_size;
  int start_sector = -1;

  tracks = FLOPPY_STX(fl, tracks);
  
  track_side_num = track | side << 7;
  sector_count = tracks[track_side_num].sector_count;
  track_sectors = tracks[track_side_num].sectors;

  for(i=0;i<sector_count;i++) {
    if(track_sectors[i].nr == sector) {
      start_sector = i;
    }
  }

  if(start_sector == -1) {
    return FLOPPY_ERROR;
  }

  DEBUG("ReadSec: T: %d  Sd: %d  S: %d  C: %d", track, side, sector, count);
  
  dst_pos = addr;
  
  for(i=0;i<count;i++) {
    sec_num = start_sector + i;
    if(sec_num > sector_count) return FLOPPY_ERROR;
    sec_size = track_sectors[sec_num].size;
    fuzzy_mask = track_sectors[sec_num].fuzzy_mask;
    for(j=0;j<sec_size;j++) {
      BYTE data;
      data = track_sectors[sec_num].data[j];
      if(track_sectors[sec_num].is_fuzzy) {
        data = (data&fuzzy_mask[j])|(rand()&~fuzzy_mask[j]);
      }
      mmu_write_byte(dst_pos, data);
      dst_pos++;
    }
  }

  return FLOPPY_OK;
}

static int write_sector(struct floppy *fl, int track, int side, int sector, LONG addr, int count)
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

/* track_data is a pointer to the entire data block. Actual sector data is located on an offset within that area.
 * fuzzy_mask is a pointer to the currently unused fuzzy_mask data block. Actual fuzzy data is always the first.
 */
static void load_sector_with_header(struct sector *sector_data, BYTE *header, BYTE *track_data, BYTE *fuzzy_mask)
{
  sector_data->offset = stx_long(header);
  sector_data->nr = header[0x0a];
  sector_data->size = 128 << (header[0x0b]&0x3);
  if(header[0x0e] & 0x80) {
    sector_data->is_fuzzy = 1;
    sector_data->fuzzy_mask = (BYTE *)malloc(sector_data->size);
    memcpy(sector_data->fuzzy_mask, fuzzy_mask, sector_data->size);
  } else {
    sector_data->is_fuzzy = 0;
  }
  sector_data->data = (BYTE *)malloc(sector_data->size);
  memcpy(sector_data->data, track_data + sector_data->offset, sector_data->size);

  DEBUG("LoadSecHdr: Nr: %d  Sz: %d  Fz: %d  Off: %d", sector_data->nr, sector_data->size, sector_data->is_fuzzy, sector_data->offset);
}

static void load_sector_without_header(struct sector *sector_data, int sector_index, BYTE *track_data)
{
  sector_data->offset = sector_index * SECSIZE;
  sector_data->nr = sector_index + 1;
  sector_data->size = SECSIZE;
  sector_data->is_fuzzy = 0; /* Without header, it will never be fuzzy */
  sector_data->data = (BYTE *)malloc(sector_data->size);
  memcpy(sector_data->data, track_data + sector_data->offset, sector_data->size);
  DEBUG("LoadSec:    Nr: %d  Sz: %d  Fz: %d  Off: %d", sector_data->nr, sector_data->size, 0, sector_data->offset);
}

static void load_track(struct floppy *fl, FILE *fp)
{
  struct track *tracks;
  int track_side_num,track_num,track_side;
  int sectors, fuzzy_size, track_image, track_size;
  BYTE header[16];
  BYTE data[50000];
  BYTE *fuzzy_pos;
  BYTE *data_pos;
  BYTE *header_pos;
  int i;

  if(fread(header, 16, 1, fp) != 1) {
    ERROR("Couldn't read from %s", FLOPPY_STX(fl, filename));
    fclose(fp);
    fl->fp = NULL;
    return;
  }

  track_side_num = header[0x0e];
  track_num = header[0x0e]&0x7f;
  track_side = (header[0x0e]&0x80) >> 7;
  track_size = stx_long(header);
  fuzzy_size = stx_long(header + 4);
  sectors = stx_word(header + 8);
  track_image = header[10] & 0xC0;

  if(sectors == 0) {
    fclose(fp);
    fl->fp = NULL;
    return;
  }

  if(fread(data, track_size - 16, 1, fp) != 1) {
    ERROR("Couldn't read from %s", FLOPPY_STX(fl, filename));
    fclose(fp);
    fl->fp = NULL;
    return;
  }

  data_pos = data;

  tracks = FLOPPY_STX(fl, tracks);
  
  tracks[track_side_num].nr = track_num;
  tracks[track_side_num].side = track_side;
  tracks[track_side_num].size = track_size;
  tracks[track_side_num].sector_count = sectors;
  tracks[track_side_num].fuzzy_size = fuzzy_size;
  /* This is a bit of a hack with the sides for now */
  tracks[track_side_num].sectors = (struct sector *)malloc(sizeof(struct sector) * sectors);

  DEBUG("LoadTrk:    T: %d  Sd: %d", track_num, track_side);

  if(header[10] & 1) {
    fuzzy_pos = data + sectors * 16;
    data_pos = fuzzy_pos + fuzzy_size;
    for(i = 0; i < sectors; i++) {
      struct sector *current_sector = &tracks[track_side_num].sectors[i];
      header_pos = data + i * 16;
      load_sector_with_header(current_sector, header_pos, data_pos, fuzzy_pos);
      if(current_sector->is_fuzzy) {
        fuzzy_pos += current_sector->size;
      }
    }
  } else {
    for(i = 0; i < sectors; i++) {
      load_sector_without_header(&tracks[track_side_num].sectors[i], i, data_pos);
    }
  }

  if(track_image & 0x40) {
    int off = 0, ts;
    BYTE *p = data_pos;
    if (track_image & 0x80) {
      off = stx_word(p);
      p += 2;
    }
    ts = stx_word(p);
    DEBUG("Track image offset %d size %d", off, ts);
  }
}

static void load_file(struct floppy *fl, FILE *fp)
{
  BYTE header[16];
  int tracks;
  int i;

  if(fread(header, 16, 1, fp) != 1) {
    ERROR("Couldn't read from %s", FLOPPY_STX(fl, filename));
    fclose(fp);
    fl->fp = NULL;
    return;
  }

  tracks = header[10];

  fl->sectors = 11;
  fl->sides = 1;
  fl->tracks = tracks;

  for(i = 0; i < tracks; i++) {
    load_track(fl, fp);
    if(!fl->fp) {
      break;
    }
  }

  if(fl->fp) {
    fclose(fp);
  }
  fl->fp = NULL;
  fl->inserted = 1;

  return;
}

void floppy_stx_init(struct floppy *fl, char *name)
{
  FILE *fp;
  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(floppy_stx, "FSTX");

  fl->image_data = (void *)malloc(sizeof(struct floppy_stx));
  fl->image_data_size = sizeof(struct floppy_stx);
  FLOPPY_STX(fl, filename) = name;

  fp = fopen(name, "rb");
  if(!fp) return;
  fl->fp = fp;
  
  load_file(fl, fp);
  fl->read_sector = read_sector;
  fl->write_sector = write_sector;
}
