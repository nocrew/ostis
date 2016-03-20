#ifndef SHIFTER_H
#define SHIFTER_H

#include "cpu.h"

void shifter_init();
void shifter_do_interrupts(struct cpu *, int);
void shifter_print_status();
void shifter_load(WORD);
void shifter_de(int);
void shifter_blank(int);
void shifter_clock(void);

#endif
