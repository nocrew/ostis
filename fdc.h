#ifndef FDC_H
#define FDC_H

#include "cpu.h"

void fdc_init();
void fdc_do_interrupts(struct cpu *);
BYTE fdc_get_register(int);
void fdc_set_register(int, BYTE);
void fdc_prepare_read(int);

#endif
