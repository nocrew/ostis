#ifndef STATE_H
#define STATE_H

#include "cpu.h"
#include "mmu.h"

struct state {
  char id[4];
  long size;
  struct cpu_state *cpu_state;
  struct mmu_state *mmu_state;
};

struct state *state_collect();
void state_restore(struct state *);
void state_clear(struct state *);
void state_save(char *, struct state *);
struct state *state_load(char *);

#endif
