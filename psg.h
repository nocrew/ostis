#ifndef PSG_H
#define PSG_H

#include "cpu.h"

void psg_init();
void psg_print_status();
void psg_do_interrupts(struct cpu *);

#endif
