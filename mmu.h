#ifndef MMU_H
#define MMU_H

#include "common.h"
#include "cpu.h"

struct mmu_state {
  struct mmu_state *next;
  char id[4];
  long size;
  char *data;
};

struct mmu {
  struct mmu *next;
  char id[4];
  const char *name;
  LONG start;
  LONG size;
  int verbosity;
  BYTE (*read_byte)(LONG);
  WORD (*read_word)(LONG);
  void (*write_byte)(LONG, BYTE);
  void (*write_word)(LONG, WORD);
  int (*state_collect)(struct mmu_state *);
  void (*state_restore)(struct mmu_state *);
  void (*diagnostics)();
  void (*interrupt)(struct cpu *);
};

struct mmu_module {
  struct mmu_module *next;
  struct mmu *module;
};

struct mmu_state *mmu_state_collect();
void mmu_state_restore(struct mmu_state *);

void mmu_init();
struct mmu *mmu_create(const char *, const char *);
void mmu_register(struct mmu *);
void mmu_print_map();
void mmu_do_interrupts(struct cpu *);

BYTE bus_read_byte(LONG);
WORD bus_read_word(LONG);
LONG bus_read_long(LONG);
BYTE bus_read_byte_print(LONG);
WORD bus_read_word_print(LONG);
LONG bus_read_long_print(LONG);
void bus_write_byte(LONG, BYTE);
void bus_write_word(LONG, WORD);
void bus_write_long(LONG, LONG);

extern int mmu_print_state;

#define MMU_WAIT_STATES()			\
  do {						\
    if(((cpu->clock + cpu->icycle) & 3) != 0) {	\
      ADD_CYCLE(2);				\
      CLOCK("Wait states: 2");			\
    }						\
  } while(0)

void mmu_clock(void);
void mmu_de(int);
void mmu_vsync(void);

#endif
