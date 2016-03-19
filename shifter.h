#ifndef SHIFTER_H
#define SHIFTER_H

#include "cpu.h"

struct resolution_data {
  void (*draw)(WORD *);
  int bitplanes;
  int border_pixels;
  void (*border)(void);
};

void shifter_init();
void shifter_do_interrupts(struct cpu *, int);
void shifter_print_status();
void shifter_load(WORD);
void shifter_border(void);

#endif
