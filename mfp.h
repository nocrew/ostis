#ifndef MFP_H
#define MFP_H

void mfp_init();
void mfp_print_status();
void mfp_do_interrupts(struct cpu *);
void mfp_do_timerb_event(struct cpu *);

void mfp_set_GPIP(int);
void mfp_clr_GPIP(int);

#endif
