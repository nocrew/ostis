#include "common.h"
#include "cpu.h"
#include "ea.h"

static void addq_b(struct cpu *cpu, int s, int mode)
{
  BYTE d,r;

  d = ea_read_byte(cpu, mode, 1);
  r = s+d;
  ea_write_byte(cpu, mode, r);

  ADD_CYCLE(4);

  cpu_set_flags_add(cpu, 0, d&0x80, r&0x80, r);
}

static void addq_w(struct cpu *cpu, int s, int mode)
{
  WORD d,r;

  d = ea_read_word(cpu, mode, 1);
  r = s+d;
  ea_write_word(cpu, mode, r);

  ADD_CYCLE(4);

  cpu_set_flags_add(cpu, 0, d&0x8000, r&0x8000, r);
}

static void addq_l(struct cpu *cpu, int s, int mode)
{
  LONG d,r;

  d = ea_read_long(cpu, mode, 1);
  r = s+d;
  ea_write_long(cpu, mode, r);

  ADD_CYCLE(8);

  cpu_set_flags_add(cpu, 0, d&0x80000000, r&0x80000000, r);
}

static void addq(struct cpu *cpu, WORD op)
{
  int s;

  ENTER;

  s = (op&0xe00)>>9;
  if(!s) s = 8;

  if(((op&0x38)>>3) == 1) {
    if(op&0x80) {
      ADD_CYCLE(8);
    } else {
      ADD_CYCLE(4);
    }
    cpu->a[op&7] += s;
    return;
  }
  
  if(op&0x38) {
    ADD_CYCLE(4);
  }

  switch((op&0xc0)>>6) {
  case 0:
    addq_b(cpu, s, op&0x3f);
    return;
  case 1:
    addq_w(cpu, s, op&0x3f);
    return;
  case 2:
    addq_l(cpu, s, op&0x3f);
    return;
  }
}

static struct cprint *addq_print(LONG addr, WORD op)
{
  int s;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  s = (op&0xe00)>>9;
  if(!s) s = 8;

  switch((op&0xc0)>>6) {
  case 0:
    strcpy(ret->instr, "ADDQ.B");
    break;
  case 1:
    strcpy(ret->instr, "ADDQ.W");
    break;
  case 2:
    strcpy(ret->instr, "ADDQ.L");
    break;
  }
  sprintf(ret->data, "#%d,", s);
  ea_print(ret, op&0x3f, 0);

  return ret;
}

void addq_init(void *instr[], void *print[])
{
  int i,d;
  
  for(d=0;d<8;d++) {
    for(i=0;i<0x40;i++) {
      if(ea_valid(i, EA_INVALID_DST|EA_INVALID_A)) {
	instr[0x5000|(d<<9)|i] = addq;
	print[0x5000|(d<<9)|i] = addq_print;
      }
      if(ea_valid(i, EA_INVALID_DST)) {
	instr[0x5040|(d<<9)|i] = addq;
	instr[0x5080|(d<<9)|i] = addq;
	print[0x5040|(d<<9)|i] = addq_print;
	print[0x5080|(d<<9)|i] = addq_print;
      }
    }
  }
}
