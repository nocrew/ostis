#include "common.h"
#include "cpu.h"
#include "ea.h"

static void add_b(struct cpu *cpu, WORD op)
{
  int rsrc,reg;
  BYTE d,r;

  rsrc = op&0x100;
  reg = (op&0xe00)>>9;

  if(rsrc) {
    d = ea_read_byte(cpu, op&0x3f, 1);
    r = cpu->d[reg]+d;
    ADD_CYCLE(8);
    ea_write_byte(cpu, op&0x3f, r);
    cpu_set_flags_add(cpu, cpu->d[reg]&0x80, d&0x80, r&0x80,   r);
  } else {
    d = ea_read_byte(cpu, op&0x3f, 0);
    r = cpu->d[reg]+d;
    ADD_CYCLE(4);
    cpu_set_flags_add(cpu, d&0x80, cpu->d[reg]&0x80, r&0x80, r);
    cpu->d[reg] = (cpu->d[reg]&0xffffff00)|r;
  }
}

static void add_w(struct cpu *cpu, WORD op)
{
  int rsrc,reg;
  WORD d,r;

  rsrc = op&0x100;
  reg = (op&0xe00)>>9;

  if(rsrc) {
    d = ea_read_word(cpu, op&0x3f, 1);
    r = cpu->d[reg]+d;
    ADD_CYCLE(8);
    ea_write_word(cpu, op&0x3f, r);
    cpu_set_flags_add(cpu, cpu->d[reg]&0x8000, d&0x8000, r&0x8000, r);
  } else {
    d = ea_read_word(cpu, op&0x3f, 0);
    r = cpu->d[reg]+d;
    ADD_CYCLE(4);
    cpu_set_flags_add(cpu, d&0x8000, cpu->d[reg]&0x8000, r&0x8000, r);
    cpu->d[reg] = (cpu->d[reg]&0xffff0000)|r;
  }
}

static void add_l(struct cpu *cpu, WORD op)
{
  int rsrc,reg;
  LONG d,r;

  rsrc = op&0x100;
  reg = (op&0xe00)>>9;

  if(rsrc) {
    d = ea_read_long(cpu, op&0x3f, 1);
    r = cpu->d[reg]+d;
    ADD_CYCLE(12);
    ea_write_long(cpu, op&0x3f, r);
    cpu_set_flags_add(cpu, cpu->d[reg]&0x80000000, d&0x80000000, r&0x80000000, r);
  } else {
    d = ea_read_long(cpu, op&0x3f, 0);
    r = cpu->d[reg]+d;
    ADD_CYCLE(6);
    cpu_set_flags_add(cpu, d&0x80000000, cpu->d[reg]&0x80000000, r&0x80000000, r);
    cpu->d[reg] = r;
  }
}

static void add(struct cpu *cpu, WORD op)
{
  ENTER;
  switch((op&0xc0)>>6) {
  case 0:
    add_b(cpu, op);
    return;
  case 1:
    add_w(cpu, op);
    return;
  case 2:
    add_l(cpu, op);
    return;
  }
}

static struct cprint *add_print(LONG addr, WORD op)
{
  int s,r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  s = (op&0xc0)>>6;
  r = (op&0xe00)>>9;

  switch(s) {
  case 0:
    strcpy(ret->instr, "ADD.B");
    break;
  case 1:
    strcpy(ret->instr, "ADD.W");
    break;
  case 2:
    strcpy(ret->instr, "ADD.L");
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

void add_init(void *instr[], void *print[])
{
  int i,r;

  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      if(ea_valid(i, EA_INVALID_A)) {
	instr[0xd000|(r<<9)|i] = add;
	print[0xd000|(r<<9)|i] = add_print;
      }
      if(ea_valid(i, EA_INVALID_NONE)) {
	instr[0xd040|(r<<9)|i] = add;
	instr[0xd080|(r<<9)|i] = add;
	print[0xd040|(r<<9)|i] = add_print;
	print[0xd080|(r<<9)|i] = add_print;
      }
      if(ea_valid(i, EA_INVALID_MEM)) {
	instr[0xd100|(r<<9)|i] = add;
	instr[0xd140|(r<<9)|i] = add;
	instr[0xd180|(r<<9)|i] = add;
	print[0xd100|(r<<9)|i] = add_print;
	print[0xd140|(r<<9)|i] = add_print;
	print[0xd180|(r<<9)|i] = add_print;
      }
    }
  }
}

