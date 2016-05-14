#ifndef UCODE_H
#define UCODE_H

typedef void (*u_sequence)(struct cpu *cpu, WORD op);
extern void u_start_sequence(u_sequence *, struct cpu *, WORD);
extern void u_end_sequence(struct cpu *, WORD);
extern void u_prefetch(struct cpu *, WORD);

typedef void (*uop_t)(void);
extern void ujump(uop_t *, int);
extern int ustep(void);

extern uop_t nop_uops[];

#endif /* UCODE_H */
