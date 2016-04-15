#ifndef UCODE_H
#define UCODE_H

typedef void (*uop_t)(void);
extern void ujump(uop_t *, int);
extern int ustep(void);

#endif /* UCODE_H */
