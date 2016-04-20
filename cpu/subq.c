#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

static void subq_b(struct cpu *cpu, int s, int mode)
{
  BYTE d,r;

  d = ea_read_byte(cpu, mode, 1);
  r = d-s;
  ea_write_byte(cpu, mode, r);

  ADD_CYCLE(4);

  cpu_set_flags_sub(cpu, 0, d&0x80, r&0x80, r);
}

static void subq_w(struct cpu *cpu, int s, int mode)
{
  WORD d,r;

  d = ea_read_word(cpu, mode, 1);
  r = d-s;
  ea_write_word(cpu, mode, r);

  ADD_CYCLE(4);

  cpu_set_flags_sub(cpu, 0, d&0x8000, r&0x8000, r);
}

static void subq_l(struct cpu *cpu, int s, int mode)
{
  LONG d,r;

  d = ea_read_long(cpu, mode, 1);
  r = d-s;
  ea_write_long(cpu, mode, r);

  ADD_CYCLE(8);

  cpu_set_flags_sub(cpu, 0, d&0x80000000, r&0x80000000, r);
}

static void subq(struct cpu *cpu, WORD op)
{
  int s;

  ENTER;

  s = (op&0xe00)>>9;
  if(!s) s = 8;

  if(((op&0x38)>>3) == 1) {
    ADD_CYCLE(8);
    cpu->a[op&7] -= s;
    cpu_prefetch();
    return;
  }
  
  if(op&0x38) {
    ADD_CYCLE(4);
  }

  switch((op&0xc0)>>6) {
  case 0:
    subq_b(cpu, s, op&0x3f);
    break;
  case 1:
    subq_w(cpu, s, op&0x3f);
    break;
  case 2:
    subq_l(cpu, s, op&0x3f);
    break;
  }
  if(!cpu->has_prefetched)
    cpu_prefetch();
}

static struct cprint *subq_print(LONG addr, WORD op)
{
  int s;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  s = (op&0xe00)>>9;
  if(!s) s = 8;

  switch((op&0xc0)>>6) {
  case 0:
    strcpy(ret->instr, "SUBQ.B");
    break;
  case 1:
    strcpy(ret->instr, "SUBQ.W");
    break;
  case 2:
    strcpy(ret->instr, "SUBQ.L");
    break;
  }
  sprintf(ret->data, "#%d,", s);
  ea_print(ret, op&0x3f, 0);

  return ret;
}

void subq_init(void *instr[], void *print[])
{
  int i,d;
  
  for(d=0;d<8;d++) {
    for(i=0;i<0x40;i++) { 
      if(ea_valid(i, EA_INVALID_DST|EA_INVALID_A)) {
	instr[0x5100|(d<<9)|i] = subq;
	print[0x5100|(d<<9)|i] = subq_print;
      }
      if(ea_valid(i, EA_INVALID_DST)) {
	instr[0x5140|(d<<9)|i] = subq;
	instr[0x5180|(d<<9)|i] = subq;
	print[0x5140|(d<<9)|i] = subq_print;
	print[0x5180|(d<<9)|i] = subq_print;
      }
    }
  }
}
