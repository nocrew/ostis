#ifndef FLOPPY_H
#define FLOPPY_H

#include "common.h"

#define FLOPPY_OK    0
#define FLOPPY_ERROR 1

struct floppy {
  FILE *fp;
  int inserted;
  int tracks;
  int sectors;
  int sides;
  int sel_trk;
  int sel_sec;
  int sel_side;
  int (*read_sector)(int track, int side, int sector, LONG addr, int count);
  int (*write_sector)(int track, int side, int sector, LONG addr, int count);
};

void floppy_side(int);
void floppy_active(int);
void floppy_sector(int);
int floppy_current_track();
int floppy_seek(int);
int floppy_seek_rel(int);
int floppy_read_sector(LONG, int);
int floppy_write_sector(LONG, int);
int floppy_read_address(LONG);
void floppy_init(char *, char *);
BYTE *floppy_allocate_memory(void);

struct floppy floppy[3];
BYTE *floppy_raw_data;
LONG floppy_raw_data_size;

#endif
