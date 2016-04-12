#ifndef CLOCK_H
#define CLOCK_H

extern void clock_step(void);
extern void clock_delay(unsigned cycles, void (*callback)(void));

#endif /* CLOCK_H */
