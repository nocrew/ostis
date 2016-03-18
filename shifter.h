#ifndef SHIFTER_H
#define SHIFTER_H

#include "cpu.h"

struct resolution_data {
  void (*draw)(WORD *);
  int bitplanes;
  long screen_cycles;
  long hblsize;
  long hblpre;
  long hblscr;
  long hblpost;
  long vblsize;
  long vblpre;
  long vblscr;
  int voff_shift;
  void (*border)(void);
};

void shifter_init();
void shifter_do_interrupts(struct cpu *, int);
void shifter_print_status();
int shifter_get_vsync();
int shifter_framecnt(int);
float shifter_fps();
void shifter_load(WORD);
void shifter_border(void);
void screen_vsync(void);

#endif
