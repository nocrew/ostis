#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

static void negx_b(struct cpu *cpu, WORD op)
{
  BYTE d,r;

  d = ea_read_byte(cpu, op&0x3f, 1);
  r = 0-d;
  if(CHKX) r -= 1;
  ea_write_byte(cpu, op&0x3f, r);

  ADD_CYCLE(4);
  if(op&0x38) {
    ADD_CYCLE(4);
  }
  cpu_set_flags_negx(cpu, d&0x80, r&0x80, r);
}

static void negx_w(struct cpu *cpu, WORD op)
{
  WORD d,r;

  d = ea_read_word(cpu, op&0x3f, 1);
  r = 0-d;
  if(CHKX) r -= 1;
  ea_write_word(cpu, op&0x3f, r);

  ADD_CYCLE(4);
  if(op&0x38) {
    ADD_CYCLE(4);
  }
  cpu_set_flags_negx(cpu, d&0x8000, r&0x8000, r);
}

static void negx_l(struct cpu *cpu, WORD op)
{
  LONG d,r;

  d = ea_read_long(cpu, op&0x3f, 1);
  r = 0-d;
  if(CHKX) r -= 1;
  ea_write_long(cpu, op&0x3f, r);

  ADD_CYCLE(6);
  if(op&0x38) {
    ADD_CYCLE(6);
  }
  cpu_set_flags_negx(cpu, d&0x80000000, r&0x80000000, r);
}

static void negx(struct cpu *cpu, WORD op)
{
  ENTER;

  switch((op&0xc0)>>6) {
  case 0:
    negx_b(cpu, op);
    return;
  case 1:
    negx_w(cpu, op);
    return;
  case 2:
    negx_l(cpu, op);
    return;
  }
}

static struct cprint *negx_print(LONG addr, WORD op)
{
  int s;

  struct cprint *ret;

  ret = cprint_alloc(addr);
  
  s = (op&0xc0)>>6;

  switch(s) {
  case 0:
    strcpy(ret->instr, "NEGX.B");
    break;
  case 1:
    strcpy(ret->instr, "NEGX.W");
    break;
  case 2:
    strcpy(ret->instr, "NEGX.L");
    break;
  }
  
  ea_print(ret, op&0x3f, s);

  return ret;
}

void negx_init(void *instr[], void *print[])
{
  int i;

  for(i=0;i<0x40;i++) {
    if(ea_valid(i, EA_INVALID_A|EA_INVALID_DST)) {
      instr[0x4000|i] = negx;
      instr[0x4040|i] = negx;
      instr[0x4080|i] = negx;
      print[0x4000|i] = negx_print;
      print[0x4040|i] = negx_print;
      print[0x4080|i] = negx_print;
    }
  }
}
