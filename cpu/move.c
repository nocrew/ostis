#include <stdio.h>
#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"
#include "ea.h"

static void move_b(struct cpu *cpu, int src, int dst)
{
  BYTE s;

  s = ea_read_byte(cpu, src, 0);
  if((dst == 0x0f) && (((src&0x38) == 0) || ((src&0x38) == 0x08))) {
    /* MEM,(...).L should not prefetch */
  } else {
    ea_set_prefetch_before_write();
  }
  ea_write_byte(cpu, dst, s);

  cpu_set_flags_move(cpu, s&0x80, s);
}

static void move_w(struct cpu *cpu, int src, int dst)
{
  WORD s;

  s = ea_read_word(cpu, src, 0);
  if((dst == 0x0f) && (((src&0x38) == 0) || ((src&0x38) == 0x08))) {
    /* MEM,(...).L should not prefetch */
  } else {
    ea_set_prefetch_before_write();
  }
  ea_write_word(cpu, dst, s);

  cpu_set_flags_move(cpu, s&0x8000, s);
}

static void move_l(struct cpu *cpu, int src, int dst)
{
  LONG s;

  s = ea_read_long(cpu, src, 0);
  if((dst == 0x0f) && (((src&0x38) == 0) || ((src&0x38) == 0x08))) {
    /* MEM,(...).L should not prefetch */
  } else {
    ea_set_prefetch_before_write();
  }
  ea_write_long(cpu, dst, s);

  cpu_set_flags_move(cpu, s&0x80000000, s);
}

static void move(struct cpu *cpu, WORD op)
{
  int src,dst;

  ENTER;

  src = op&0x3f;
  dst = MODESWAP((op&0xfc0)>>6);
  
  cpu->cyclecomp = 1;
  ADD_CYCLE(4);

  switch((op&0x3000)>>12) {
  case 1: /* BYTE */
    move_b(cpu, src, dst);
    return;
  case 2: /* LONG */
    move_l(cpu, src, dst);
    return;
  case 3: /* WORD */
    move_w(cpu, src, dst);
    return;
  }
}

static struct cprint *move_print(LONG addr, WORD op)
{
  int src,dst,s;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  src = op&0x3f;
  dst = MODESWAP((op&0xfc0)>>6);
  s = (op&0x3000)>>12;
  if(s == 1) s = 0;
  if(s == 3) s = 1;

  switch(s) {
  case 0:
    strcpy(ret->instr, "MOVE.B");
    break;
  case 1:
    strcpy(ret->instr, "MOVE.W");
    break;
  case 2:
    strcpy(ret->instr, "MOVE.L");
    break;
  }
  ea_print(ret, src, s);
  strcat(ret->data, ",");
  ea_print(ret, dst, s);
  
  return ret;
}

void move_init(void *instr[], void *print[])
{
  int i;

  for(i=0x1000;i<0x4000;i++) {
    if(ea_valid(i&0x3f, EA_INVALID_NONE) && 
       ea_valid(MODESWAP((i&0xfc0)>>6), EA_INVALID_A|EA_INVALID_DST)) {
      instr[i] = move;
      print[i] = move_print;
    }
  }
}
