#ifndef FLOPPY_H
#define FLOPPY_H

#define FLOPPY_OK    0
#define FLOPPY_ERROR 1

void floppy_side(int);
void floppy_active(int);
int floppy_seek(int);
int floppy_seek_rel(int);
int floppy_read_sector(LONG, int);
void floppy_init(char *);

#endif
