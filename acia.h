#ifndef ACIA_H
#define ACIA_H

#include "cpu.h"

void acia_init(void);
int acia_rx_full(void);
void acia_rx_data(BYTE);

#endif
