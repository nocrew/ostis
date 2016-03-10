#ifndef MFP_H
#define MFP_H

#include "common.h"

#define MFP_GPIP_ACIA 4
#define MFP_GPIP_FDC 5
#define MFP_GPIP_HDC 5

void mfp_init();
void mfp_print_status();
void mfp_set_pending(int);
void mfp_do_interrupts(struct cpu *);
void mfp_do_timerb_event(struct cpu *);

void mfp_set_GPIP(int);
void mfp_clr_GPIP(int);

BYTE mfp_get_ISRB();

#endif
