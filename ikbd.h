#ifndef IKBD_H
#define IKBD_H

#include "cpu.h"

void ikbd_init();
void ikbd_queue_key(int, int);
void ikbd_do_interrupts(struct cpu *);

#endif
