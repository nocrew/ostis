#include "common.h"
#include "cpu.h"
#include "ea.h"

static void not_b(struct cpu *cpu, WORD op)
{
  BYTE r;

  ADD_CYCLE(4);
  if(op&0x38) {
    ADD_CYCLE(4);
  }
  r = ~ea_read_byte(cpu, op&0x3f, 1);
  ea_write_byte(cpu, op&0x3f, r);
  cpu_set_flags_move(cpu, r&0x80, r);
}

static void not_w(struct cpu *cpu, WORD op)
{
  WORD r;

  ADD_CYCLE(4);
  if(op&0x38) {
    ADD_CYCLE(4);
  }
  r = ~ea_read_word(cpu, op&0x3f, 1);
  ea_write_word(cpu, op&0x3f, r);
  cpu_set_flags_move(cpu, r&0x8000, r);
}

static void not_l(struct cpu *cpu, WORD op)
{
  LONG r;

  ADD_CYCLE(6);
  if(op&0x38) {
    ADD_CYCLE(6);
  }
  r = ~ea_read_long(cpu, op&0x3f, 1);
  ea_write_long(cpu, op&0x3f, r);
  cpu_set_flags_move(cpu, r&0x80000000, r);
}

static void not(struct cpu *cpu, WORD op)
{
  ENTER;

  switch((op&0xc0)>>6) {
  case 0:
    not_b(cpu, op);
    return;
  case 1:
    not_w(cpu, op);
    return;
  case 2:
    not_l(cpu, op);
    return;
  }
}

static struct cprint *not_print(LONG addr, WORD op)
{
  int s;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  s = (op&0xc0)>>6;
  
  switch(s) {
  case 0:
    strcpy(ret->instr, "NOT.B");
    break;
  case 1:
    strcpy(ret->instr, "NOT.W");
    break;
  case 2:
    strcpy(ret->instr, "NOT.L");
    break;
  }

  ea_print(ret, op&0x3f, s);

  return ret;
}

void not_init(void *instr[], void *print[])
{
  int i;

  for(i=0;i<0xc0;i++) {
    instr[0x4600|i] = not;
    print[0x4600|i] = not_print;
  }
}
