#ifndef STATE_H
#define STATE_H

#include "common.h"
#include "cpu.h"
#include "mmu.h"

struct state {
  long id;
  long version;
  long size;
  uint64_t timestamp;
  long delta;
  struct cpu_state *cpu_state;
  struct mmu_state *mmu_state;
};

struct state_list {
  struct state_list *next;
  struct state *state;
};

#define STATE_INVALID 0
#define STATE_VALID 1

#define STATE_VERSION 2

struct state *state_collect();
void state_restore(struct state *);
void state_clear(struct state *);
void state_save(char *, struct state *);
struct state *state_load(char *);
int state_valid_id(char *);
void state_write_mem_byte(char *, BYTE);
void state_write_mem_word(char *, WORD);
void state_write_mem_long(char *, LONG);
BYTE state_read_mem_byte(char *);
WORD state_read_mem_word(char *);
LONG state_read_mem_long(char *);

#endif
