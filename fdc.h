#ifndef FDC_H
#define FDC_H

#include "cpu.h"

void fdc_init();
void fdc_do_interrupts(struct cpu *);

#endif
