#ifndef IKBD_H
#define IKBD_H

void ikbd_init();
void ikbd_queue_key(int, int);
void ikbd_do_interrupts();

#endif
