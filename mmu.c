#include <stdio.h>
#include "common.h"
#include "cpu.h"
#include "mfp.h"
#include "shifter.h"
#include "mmu.h"

static struct mmu *memory = NULL;

static struct mmu *find_entry(struct mmu *data)
{
  struct mmu *t;

  t = memory;

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

#if 0
  if((opsize != OPSIZE_BYTE) && (addr&1)) {
    mmu_send_address_error();
  }
#endif
  
  t = memory;
  
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
#if 0
  mmu_write_long(0x420, 0x752019f3);
  mmu_write_long(0x43a, 0x237698aa);
  mmu_write_long(0x51a, 0x5555aaaa);
  mmu_write_byte(0x424, 10);
  mmu_write_byte(0xff8001, 10);
  mmu_write_long(0x436, 0x400000-0x8000);
  mmu_write_long(0x42e, 0x400000);
 
  mmu_write_word(0x446, 0); /* Boot from A */
  mmu_write_word(0x4a6, 2); /* A and B drives */
  mmu_write_long(0x4c2, 0x03); /* only A and B */
#endif
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
  if(!data) return;
  
  if(!find_entry(data)) {
    data->next = memory;
    memory = data;
  }
}

void mmu_print_map()
{
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
}

void mmu_do_interrupts(struct cpu *cpu)
{
  mfp_do_interrupts(cpu);
  shifter_do_interrupts(cpu);
}
