#include "common.h"
#include "cpu.h"
#include "ea.h"

static void and_b(struct cpu *cpu, WORD op)
{
  BYTE r;
  int reg;

  reg = (op&0xe00)>>9;

  if(op&0x100) {
    ADD_CYCLE(8);
    r = cpu->d[reg] & ea_read_byte(cpu, op&0x3f, 1);
    ea_write_byte(cpu, op&0x3f, r);
  } else {
    ADD_CYCLE(4);
    r = cpu->d[reg] & ea_read_byte(cpu, op&0x3f, 0);
    cpu->d[reg] = (cpu->d[reg]&0xffffff00)|r;
  }
  cpu_set_flags_move(cpu, r&0x80, r);
}

static void and_w(struct cpu *cpu, WORD op)
{
  WORD r;
  int reg;

  reg = (op&0xe00)>>9;

  if(op&0x100) {
    ADD_CYCLE(8);
    r = cpu->d[reg] & ea_read_word(cpu, op&0x3f, 1);
    ea_write_word(cpu, op&0x3f, r);
  } else {
    ADD_CYCLE(4);
    r = cpu->d[reg] & ea_read_word(cpu, op&0x3f, 0);
    cpu->d[reg] = (cpu->d[reg]&0xffff0000)|r;
  }
  cpu_set_flags_move(cpu, r&0x8000, r);
}

static void and_l(struct cpu *cpu, WORD op)
{
  LONG r;
  int reg;

  reg = (op&0xe00)>>9;

  if(op&0x100) {
    ADD_CYCLE(12);
    r = cpu->d[reg] & ea_read_long(cpu, op&0x3f, 1);
    ea_write_long(cpu, op&0x3f, r);
  } else {
    ADD_CYCLE(6);
    r = cpu->d[reg] & ea_read_long(cpu, op&0x3f, 0);
    cpu->d[reg] = r;
  }
  cpu_set_flags_move(cpu, r&0x80000000, r);
}

static void and(struct cpu *cpu, WORD op)
{
  ENTER;

  switch((op&0xc0)>>6) {
  case 0:
    and_b(cpu, op);
    return;
  case 1:
    and_w(cpu, op);
    return;
  case 2:
    and_l(cpu, op);
    return;
  }
}

static struct cprint *and_print(LONG addr, WORD op)
{
  int s,r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  s = (op&0xc0)>>6;
  r = (op&0xe00)>>9;
  
  switch(s) {
  case 0:
    strcpy(ret->instr, "AND.B");
    break;
  case 1:
    strcpy(ret->instr, "AND.W");
    break;
  case 2:
    strcpy(ret->instr, "AND.L");
    break;
  }

  if(op&0x100) {
    sprintf(ret->data, "D%d,", r);
    ea_print(ret, op&0x3f, s);
  } else {
    ea_print(ret, op&0x3f, s);
    sprintf(ret->data, "%s,D%d", ret->data, r);
  }

  return ret;
}

void and_init(void *instr[], void *print[])
{
  int i,r;

  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      if(ea_valid(i, EA_INVALID_A)) {
	instr[0xc000|(r<<9)|i] = and;
	instr[0xc040|(r<<9)|i] = and;
	instr[0xc080|(r<<9)|i] = and;
	print[0xc000|(r<<9)|i] = and_print;
	print[0xc040|(r<<9)|i] = and_print;
	print[0xc080|(r<<9)|i] = and_print;
      }
      if(ea_valid(i, EA_INVALID_MEM)) {
	instr[0xc100|(r<<9)|i] = and;
	instr[0xc140|(r<<9)|i] = and;
	instr[0xc180|(r<<9)|i] = and;
	print[0xc100|(r<<9)|i] = and_print;
	print[0xc140|(r<<9)|i] = and_print;
	print[0xc180|(r<<9)|i] = and_print;
      }
    }
  }
}
