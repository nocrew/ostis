#include "common.h"
#include "cpu.h"
#include "ea.h"

static void tst_b(struct cpu *cpu, WORD op)
{
  BYTE s;

  s = ea_read_byte(cpu, op&0x3f, 0);
  cpu_set_flags_move(cpu, s&0x80, s);
  ADD_CYCLE(4);
}

static void tst_w(struct cpu *cpu, WORD op)
{
  WORD s;

  s = ea_read_word(cpu, op&0x3f, 0);
  cpu_set_flags_move(cpu, s&0x8000, s);
  ADD_CYCLE(4);
}

static void tst_l(struct cpu *cpu, WORD op)
{
  LONG s;

  s = ea_read_long(cpu, op&0x3f, 0);
  cpu_set_flags_move(cpu, s&0x80000000, s);
  ADD_CYCLE(4);
}

static void tst(struct cpu *cpu, WORD op)
{
  ENTER;

  switch((op&0xc0)>>6) {
  case 0:
    tst_b(cpu, op);
    return;
  case 1:
    tst_w(cpu, op);
    return;
  case 2:
    tst_l(cpu, op);
    return;
  }
}

static struct cprint *tst_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  switch((op&0xc0)>>6) {
  case 0:
    strcpy(ret->instr, "TST.B");
    break;
  case 1:
    strcpy(ret->instr, "TST.W");
    break;
  case 2:
    strcpy(ret->instr, "TST.L");
    break;
  }
  ea_print(ret, op&0x3f, 0);

  return ret;
}

void tst_init(void *instr[], void *print[])
{
  int i;
  
  for(i=0;i<0xc0;i++) {
    instr[0x4a00|i] = tst;
    print[0x4a00|i] = tst_print;
  }
}
