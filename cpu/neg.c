#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

static void neg_b(struct cpu *cpu, WORD op)
{
  BYTE b;
  int r;

  ADD_CYCLE(4);
  if(op&0x38) {
    ADD_CYCLE(4);
  }
  b = ea_read_byte(cpu, op&0x3f, 1);
  r = (int)*((SBYTE *)&b);
  r = -r;
  ea_write_byte(cpu, op&0x3f, r);
  cpu_set_flags_neg(cpu, b&0x80, r&0x80, r);
}

static void neg_w(struct cpu *cpu, WORD op)
{
  WORD w;
  int r;

  ADD_CYCLE(4);
  if(op&0x38) {
    ADD_CYCLE(4);
  }
  w = ea_read_word(cpu, op&0x3f, 1);
  r = (int)*((SWORD *)&w);
  r = -r;
  ea_write_word(cpu, op&0x3f, r);
  cpu_set_flags_neg(cpu, w&0x8000, r&0x8000, r);
}

static void neg_l(struct cpu *cpu, WORD op)
{
  LONG l;
  int r;

  ADD_CYCLE(6);
  if(op&0x38) {
    ADD_CYCLE(6);
  }
  l = ea_read_long(cpu, op&0x3f, 1);
  r = (int)*((SLONG *)&l);
  r = -r;
  ea_write_long(cpu, op&0x3f, r);
  cpu_set_flags_neg(cpu, l&0x80000000, r&0x80000000, r);
}

static void neg(struct cpu *cpu, WORD op)
{
  ENTER;

  switch((op&0xc0)>>6) {
  case 0:
    neg_b(cpu, op);
    return;
  case 1:
    neg_w(cpu, op);
    return;
  case 2:
    neg_l(cpu, op);
    return;
  }
}

static struct cprint *neg_print(LONG addr, WORD op)
{
  int s;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  s = (op&0xc0)>>6;
  
  switch(s) {
  case 0:
    strcpy(ret->instr, "NEG.B");
    break;
  case 1:
    strcpy(ret->instr, "NEG.W");
    break;
  case 2:
    strcpy(ret->instr, "NEG.L");
    break;
  }

  ea_print(ret, op&0x3f, s);

  return ret;
}

void neg_init(void *instr[], void *print[])
{
  int i;

  for(i=0;i<0xc0;i++) {
    if(ea_valid(i, EA_INVALID_DST|EA_INVALID_A)) {
      instr[0x4400|i] = neg;
      print[0x4400|i] = neg_print;
    }
  }
}
