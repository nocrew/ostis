#ifndef SHIFTER_H
#define SHIFTER_H

#include "cpu.h"

void shifter_init();
void shifter_do_interrupts(struct cpu *);
void shifter_build_image();
void shifter_print_status();

#endif
