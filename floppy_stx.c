#include "common.h"
#include "mmu.h"
#include "floppy.h"
#include "floppy_stx.h"

static struct floppy *fl;
static char *filename = NULL;
//static BYTE *raw_data;
//static LONG raw_data_size;

#define MAXSIDES 2
#define MAXTRACKS 86

struct sector {
  int nr;
  int size;
  int offset;
  BYTE *data;
};

struct track {
  int nr;
  int size;
  int fuzzy;
  int sector_count;
  int sides;
  struct sector *sectors[MAXSIDES];
};

static struct track tracks[MAXTRACKS];
  
#define SECSIZE 512

static int read_sector(int track, int side, int sector, LONG addr, int count)
{
  int i,j,dst_pos;
  struct sector *track_sectors;
  int sector_count;
  int sec_num, sec_size;
  int start_sector = -1;

  sector_count = tracks[track].sector_count;
  track_sectors = tracks[track].sectors[side];

  for(i=0;i<sector_count;i++) {
    if(track_sectors[i].nr == sector) {
      start_sector = i;
    }
  }

  if(start_sector == -1) {
    return FLOPPY_ERROR;
  }

  dst_pos = addr;
  
  for(i=0;i<count;i++) {
    sec_num = start_sector + i;
    if(sec_num > sector_count) return FLOPPY_ERROR;
    sec_size = track_sectors[sec_num].size;
    for(j=0;j<sec_size;j++) {
      mmu_write_byte(dst_pos, track_sectors[sec_num].data[j]);
      dst_pos++;
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

static void load_sector(int sector_index, int track, int side, int sector, int size, BYTE *data, struct sector *sector_data)
{
  sector_data->nr = sector;
  sector_data->size = size;

  sector_data->data = (BYTE *)malloc(size);
  
  memcpy(sector_data->data, data, size);
}

static void load_track(FILE *fp)
{
  int track_num;
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

  track_num = sector_track[0];
  tracks[track_num].nr = track_num;
  tracks[track_num].size = track_size;
  tracks[track_num].sector_count = sectors;
  tracks[track_num].fuzzy = fuzzy;
  /* This is a bit of a hack with the sides for now */
  tracks[track_num].sectors[0] = (struct sector *)malloc(sizeof(struct sector) * sectors);
  tracks[track_num].sectors[1] = (struct sector *)malloc(sizeof(struct sector) * sectors);
  
  for(i = 0; i < sectors; i++) {
    tracks[track_num].sectors[sector_side[i]][i].offset = sector_offset[i];
    load_sector(i, sector_track[i], sector_side[i], sector_nr[i], sector_size[i],
		pos + sector_offset[i], &tracks[track_num].sectors[sector_side[i]][i]);
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
