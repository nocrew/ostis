#include <stdio.h>
#include "common.h"
#include "cpu.h"
#include "mfp.h"
#include "shifter.h"
#include "ikbd.h"
#include "fdc.h"
#include "mmu.h"

static struct mmu *memory[65536];

static struct mmu *find_entry(struct mmu *data, int block)
{
  struct mmu *t;

  t = memory[block];

  while(t) {
    if((t->start == data->start) &&
       (t->size == data->size))
      return t;
    t = t->next;
  }

  return NULL;
}

static struct mmu *dispatch(LONG addr)
{
  struct mmu *t;
  int off;

  off = (addr&0xffff00)>>8;
  if(!memory[off]) return NULL;
  if(!memory[off]->next) return memory[off];

  t = memory[off];
  
  while(t) {
    if((addr >= t->start) &&
       (addr < (t->start + t->size))) {
      return t;
    }
    t = t->next;
  }

  return NULL;
}

void mmu_init()
{
  int i;
  
  for(i=0;i<65536;i++) {
    memory[i] = NULL;
  }
}

void mmu_send_bus_error(LONG addr)
{
  fprintf(stdout, "BUS ERROR at 0x%08x\n", addr);
  cpu_print_status();
  cpu_set_exception(2);
}

BYTE mmu_read_byte_print(LONG addr)
{
  struct mmu *t;

  addr &= 0xffffff;

  if(addr >= 0xff8000) return 0x2a;

  t = dispatch(addr);
  if(!t) {
    return 0;
  }
  if(!t->read_byte) {
    return 0;
  }
  return t->read_byte(addr);
}

WORD mmu_read_word_print(LONG addr)
{
  struct mmu *t;

  addr &= 0xffffff;

  if(addr >= 0xff8000) return 0x2a2a;

  t = dispatch(addr);
  if(!t) {
    return 0;
  }
  if(!t->read_word) {
    return 0;
  }
  return t->read_word(addr);
}

LONG mmu_read_long_print(LONG addr)
{
  struct mmu *t;

  addr &= 0xffffff;

  if(addr >= 0xff8000) return 0x2a2a2a2a;

  t = dispatch(addr);
  if(!t) {
    return 0;
  }
  if(!t->read_long) {
    return 0;
  }
  return t->read_long(addr);
}

BYTE mmu_read_byte(LONG addr)
{
  struct mmu *t;

  addr &= 0xffffff;

  t = dispatch(addr);
  if(!t) {
    mmu_send_bus_error(addr);
    return 0;
  }
  if(!t->read_byte) {
    mmu_send_bus_error(addr);
    return 0;
  }
  return t->read_byte(addr);
}

WORD mmu_read_word(LONG addr)
{
  struct mmu *t;

  addr &= 0xffffff;

  t = dispatch(addr);
  if(!t) {
    mmu_send_bus_error(addr);
    return 0;
  }
  if(!t->read_word) {
    mmu_send_bus_error(addr);
    return 0;
  }
  return t->read_word(addr);
}

LONG mmu_read_long(LONG addr)
{
  struct mmu *t;

  addr &= 0xffffff;

  t = dispatch(addr);
  if(!t) {
    mmu_send_bus_error(addr);
    return 0;
  }
  if(!t->read_long) {
    mmu_send_bus_error(addr);
    return 0;
  }
  return t->read_long(addr);
}

void mmu_write_byte(LONG addr, BYTE data)
{
  struct mmu *t;

  addr &= 0xffffff;

  t = dispatch(addr);
  if(!t) {
    mmu_send_bus_error(addr);
    return;
  }
  if(!t->write_byte) {
    mmu_send_bus_error(addr);
    return;
  }
  t->write_byte(addr, data);
}

void mmu_write_word(LONG addr, WORD data)
{
  struct mmu *t;

  addr &= 0xffffff;

  t = dispatch(addr);
  if(!t) {
    mmu_send_bus_error(addr);
    return;
  }
  if(!t->write_word) {
    mmu_send_bus_error(addr);
    return;
  }
  t->write_word(addr, data);
}

void mmu_write_long(LONG addr, LONG data)
{
  struct mmu *t;

  addr &= 0xffffff;

  t = dispatch(addr);
  if(!t) {
    mmu_send_bus_error(addr);
    return;
  }
  if(!t->write_long) {
    mmu_send_bus_error(addr);
    return;
  }
  t->write_long(addr, data);
}

void mmu_register(struct mmu *data)
{
  int i,off;

  if(!data) return;
  
  for(i=0;i<data->size;i++) {
    off = ((i+data->start)&0xffff00)>>8;
    if(find_entry(data, off) != data) {
      data->next = memory[off];
      memory[off] = data;
    }
  }
}

void mmu_print_map()
{
#if 0 /* Unable to print after dispatch change */
  struct mmu *t;

  t = memory;
  
  while(t) {
    printf("Name : %s\n", t->name);
    printf("Start: 0x%08x\n", t->start);
    printf("End  : 0x%08x\n", t->start+t->size-1);
    printf("Size : %d\n", t->size);
    printf("\n");
    t = t->next;
  }
#endif
}

void mmu_do_interrupts(struct cpu *cpu)
{
  mfp_do_interrupts(cpu);
  fdc_do_interrupts(cpu);
  ikbd_do_interrupts();
}

