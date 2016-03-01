#ifndef SHIFTER_H
#define SHIFTER_H

#include "cpu.h"

struct resolution_data {
  long hblsize;
  long hblpre;
  long hblscr;
  long hblpost;
  long vblsize;
  long vblpre;
  long vblscr;
  int voff_shift;
  int border;
};

void shifter_init();
void shifter_do_interrupts(struct cpu *, int);
void shifter_build_image(int);
void shifter_print_status();
int shifter_on_display(int rasterpos);
int shifter_get_vsync();
int shifter_framecnt(int);
void shifter_force_gen_picture();

#endif
