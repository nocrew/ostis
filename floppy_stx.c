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
  int track_data_size; /* Can differ from total size */
  BYTE *track_data;
};

struct floppy_stx {
  struct track tracks[MAXTRACKS+128]; /* Both sides */
};
  
#define FLOPPY_STX(f, x) ((struct floppy_stx *)f->image_data)->x
#define SECSIZE 512
#define TRKSIZE 6250

/* Indexes for generating tracks */
#define TRKGEN_SEC9  0
#define TRKGEN_SEC10 1
#define TRKGEN_SEC11 2

#define TRKGEN_GAP1   0 /* 0x4e */
#define TRKGEN_GAP2A  1 /* 0x00 */
#define TRKGEN_GAP2B  2 /* 0xa1 */
#define TRKGEN_GAP3A  3 /* 0x4e */
#define TRKGEN_GAP3B1 4 /* 0x00 */
#define TRKGEN_GAP3B2 5 /* 0xa1 */
#define TRKGEN_GAP4   6 /* 0x4e */
#define TRKGEN_GAP5   7 /* 0x4e */

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

static int read_track(struct floppy *fl, int track_num, int side, LONG addr, int dma_count)
{
  struct track *tracks,*track;
  int track_side_num;
  int size;
  int i;

  DEBUG("ReadTrk: T: %d  Sd: %d  A: %06x  C: %d", track_num, side, addr, dma_count);
  
  tracks = FLOPPY_STX(fl, tracks);
  track_side_num = track_num | side << 7;
  track = &tracks[track_side_num];
  if(!track) {
    return FLOPPY_ERROR;
  }

  size = track->track_data_size;

  /* Do not read more than was requested by DMA */
  if(dma_count * SECSIZE < size) {
    size = dma_count * SECSIZE;
  }

  /* Do not read more than a maximum track */
  if(size > TRKSIZE) {
    size = TRKSIZE;
  }
  
  for(i=0;i<size;i++) {
    mmu_write_byte(addr + i, track->track_data[i]);
  }
  return FLOPPY_OK;
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
    sector_data->fuzzy_mask = malloc(sector_data->size);
    memcpy(sector_data->fuzzy_mask, fuzzy_mask, sector_data->size);
  } else {
    sector_data->is_fuzzy = 0;
  }
  sector_data->data = malloc(sector_data->size);
  memcpy(sector_data->data, track_data + sector_data->offset, sector_data->size);

  DEBUG("LoadSecHdr: Nr: %d  Sz: %d  Fz: %d  Off: %d", sector_data->nr, sector_data->size, sector_data->is_fuzzy, sector_data->offset);
}

static void load_sector_without_header(struct sector *sector_data, int sector_index, BYTE *track_data)
{
  sector_data->offset = sector_index * SECSIZE;
  sector_data->nr = sector_index + 1;
  sector_data->size = SECSIZE;
  sector_data->is_fuzzy = 0; /* Without header, it will never be fuzzy */
  sector_data->data = malloc(sector_data->size);
  memcpy(sector_data->data, track_data + sector_data->offset, sector_data->size);
  DEBUG("LoadSec:    Nr: %d  Sz: %d  Fz: %d  Off: %d", sector_data->nr, sector_data->size, 0, sector_data->offset);
}

static WORD crc16(int size, BYTE *buffer)
{
  int i,j,byte;
  WORD crc = 0xffff;

  for(i=0;i<size;i++) {
    byte = buffer[i];
    crc ^= byte<<8;
    for(j=0;j<8;j++) {
      if(crc&0x8000) {
        crc = (crc<<1)^0x1021;
      } else {
        crc <<= 1;
      }
      byte <<= 1;
    }
  }
  return crc;
}

static void generate_track_data(int track, int side, BYTE *buffer, int sector_count, struct sector *sectors)
{
  int structure[3][8] = {
    {60, 12, 3, 22, 12, 3, 40, 664}, /*  9 sectors */
    {60, 12, 3, 22, 12, 3, 40, 50},  /* 10 sectors */
    {10,  3, 3, 22, 12, 3,  1, 20}   /* 11 sectors */
  };
  int i,sec;
  WORD crc;
  int strnum;
  int bufpos = 0;
  BYTE *data;

  if(sector_count < 9 || sector_count > 11) {
    FATAL("Unable to generate track data for disks with other sector counts than 9, 10 or 11");
  }

  strnum = sector_count-9;

  /* Gap 1 */
  for(i=0;i<structure[strnum][TRKGEN_GAP1];i++) {
    buffer[bufpos++] = 0x4e;
  }

  /* For each sector: */
  for(sec=0;sec<sector_count;sec++) {
    data = sectors[sec].data;
    
    /* Gap 2 */
    for(i=0;i<structure[strnum][TRKGEN_GAP2A];i++) {
      buffer[bufpos++] = 0x00;
    }
    for(i=0;i<structure[strnum][TRKGEN_GAP2B];i++) {
      buffer[bufpos++] = 0xa1;
    }

    /* Index */
    buffer[bufpos++] = 0xfe;
    buffer[bufpos++] = track;
    buffer[bufpos++] = side;
    buffer[bufpos++] = sec + 1;
    buffer[bufpos++] = 2; /* 512 bytes */
    crc = crc16(8, &buffer[bufpos]-8);
    buffer[bufpos++] = (crc>>8)&0xff;
    buffer[bufpos++] = crc&0xff;

    /* Gap 3 */
    for(i=0;i<structure[strnum][TRKGEN_GAP3A];i++) {
      buffer[bufpos++] = 0x4e;
    }
    for(i=0;i<structure[strnum][TRKGEN_GAP3B1];i++) {
      buffer[bufpos++] = 0x00;
    }
    for(i=0;i<structure[strnum][TRKGEN_GAP3B2];i++) {
      buffer[bufpos++] = 0xa1;
    }

    /* Data */
    buffer[bufpos++] = 0xfb;
    for(i=0;i<SECSIZE;i++) {
      buffer[bufpos++] = data[i];
    }
    crc = crc16(SECSIZE, data);
    buffer[bufpos++] = (crc>>8)&0xff;
    buffer[bufpos++] = crc&0xff;
    
    /* Gap 4 */
    for(i=0;i<structure[strnum][TRKGEN_GAP4];i++) {
      buffer[bufpos++] = 0x4e;
    }
  }
  /* Gap 5 */
  for(i=0;i<structure[strnum][TRKGEN_GAP5];i++) {
    buffer[bufpos++] = 0x4e;
  }

  if(bufpos != TRKSIZE) {
    FATAL("Generated track size was %d bytes, not %d. This is fatal.", bufpos, TRKSIZE);
  }
}

static void load_track(struct floppy *fl, FILE *fp)
{
  struct track *tracks;
  int track_side_num,track_num,track_side;
  int sectors, fuzzy_size, track_image, track_size, track_data_size;
  BYTE header[16] = {};
  BYTE data[50000];
  BYTE *fuzzy_pos;
  BYTE *data_pos;
  BYTE *header_pos;
  int track_image_offset = 0;
  int i;

  if(fread(header, 16, 1, fp) != 1) {
    ERROR("Couldn't read from %s", fl->filename);
    fclose(fp);
    fl->fp = NULL;
    return;
  }

  track_side_num = header[0x0e];
  track_num = header[0x0e]&0x7f;
  track_side = (header[0x0e]&0x80) >> 7;
  track_size = stx_long(header);
  track_data_size = stx_word(&header[0x0c]);
  fuzzy_size = stx_long(header + 4);
  sectors = stx_word(header + 8);
  track_image = header[10] & 0xC0;

  if(sectors == 0) {
    fclose(fp);
    fl->fp = NULL;
    return;
  }

  if(fread(data, track_size - 16, 1, fp) != 1) {
    ERROR("Couldn't read from %s", fl->filename);
    fclose(fp);
    fl->fp = NULL;
    return;
  }

  data_pos = data;

  tracks = FLOPPY_STX(fl, tracks);
  
  tracks[track_side_num].nr = track_num;
  tracks[track_side_num].side = track_side;
  tracks[track_side_num].size = track_size;
  tracks[track_side_num].track_data_size = track_data_size;
  tracks[track_side_num].sector_count = sectors;
  tracks[track_side_num].fuzzy_size = fuzzy_size;
  tracks[track_side_num].track_data = NULL;
  tracks[track_side_num].sectors = malloc(sizeof(struct sector) * sectors);

  fuzzy_pos = data;
  if(header[10]&1) {
    fuzzy_pos += sectors * 16;
  }
  data_pos = fuzzy_pos + fuzzy_size;

  DEBUG("LoadTrk:    T: %d  Sd: %d  Fl: %02x  Tsz: %d", track_num, track_side, header[10], track_data_size);

  if(track_image & 0x40) {
    BYTE *ti_pos = data_pos;
    if (track_image & 0x80) {
      track_image_offset = stx_word(ti_pos);
      ti_pos += 2;
    }
    track_data_size = stx_word(ti_pos);
    TRACE("TI: Off: %d  Tsz: %d", track_image_offset, track_data_size);
    tracks[track_side_num].track_data = malloc(track_data_size);
    memcpy(tracks[track_side_num].track_data, data_pos, track_data_size);
  }
  
  if(header[10] & 1) {
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

  if(!tracks[track_side_num].track_data) {
    tracks[track_side_num].track_data = malloc(TRKSIZE);
    generate_track_data(track_num, track_side, tracks[track_side_num].track_data, sectors, tracks[track_side_num].sectors);
  }
}

static void load_file(struct floppy *fl, FILE *fp)
{
  BYTE header[16];
  int tracks;
  int i;

  if(fread(header, 16, 1, fp) != 1) {
    ERROR("Couldn't read from %s", fl->filename);
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

void floppy_stx_init(struct floppy *fl)
{
  FILE *fp;
  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(floppy_stx, "FSTX");

  fl->image_data = (void *)calloc(1, sizeof(struct floppy_stx));

  fp = fopen(fl->filename, "rb");
  if(!fp) return;
  fl->fp = fp;
  
  load_file(fl, fp);
  fl->read_sector = read_sector;
  fl->write_sector = write_sector;
  fl->read_track = read_track;
}
