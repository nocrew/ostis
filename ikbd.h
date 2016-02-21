#ifndef IKBD_H
#define IKBD_H

#include "cpu.h"

#define IKBD_JOY_UP    0x01
#define IKBD_JOY_DOWN  0x02
#define IKBD_JOY_LEFT  0x04
#define IKBD_JOY_RIGHT 0x08
#define IKBD_JOY_FIRE  0x80

void ikbd_init();
void ikbd_queue_key(int, int);
void ikbd_queue_motion(int, int);
void ikbd_button(int, int);
void ikbd_joystick(int direction);
void ikbd_fire(int state);
void ikbd_do_interrupts(struct cpu *);
void ikbd_print_status();

#endif
