#ifndef MMU_H
#define MMU_H

#include "common.h"
#include "cpu.h"

struct mmu {
  struct mmu *next;
  char *name;
  LONG start;
  LONG size;
  BYTE (*read_byte)(LONG);
  WORD (*read_word)(LONG);
  LONG (*read_long)(LONG);
  void (*write_byte)(LONG, BYTE);
  void (*write_word)(LONG, WORD);
  void (*write_long)(LONG, LONG);
};

void mmu_init();
void mmu_register(struct mmu *);
void mmu_send_bus_error(LONG);
void mmu_print_map();
void mmu_do_interrupts(struct cpu *);

BYTE mmu_read_byte(LONG);
WORD mmu_read_word(LONG);
LONG mmu_read_long(LONG);
BYTE mmu_read_byte_print(LONG);
WORD mmu_read_word_print(LONG);
LONG mmu_read_long_print(LONG);
void mmu_write_byte(LONG, BYTE);
void mmu_write_word(LONG, WORD);
void mmu_write_long(LONG, LONG);


#endif
