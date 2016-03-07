#ifndef DMA_H
#define DMA_H

#include "common.h"
#include "cpu.h"

void dma_init();
void dma_do_interrupts(struct cpu *);

#endif
