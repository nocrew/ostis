#include "common.h"
#include "cpu.h"
#include "ea.h"

static void sub_b(struct cpu *cpu, WORD op)
{
  int rsrc,reg;
  BYTE d,r;

  rsrc = op&0x100;
  reg = (op&0xe00)>>9;

  if(rsrc) {
    d = ea_read_byte(cpu, op&0x3f, 1);
    ADD_CYCLE(8);
    r = d-(cpu->d[reg]&0xff);
    ea_write_byte(cpu, op&0x3f, r);
    cpu_set_flags_sub(cpu, cpu->d[reg]&0x80, d&0x80, r&0x80, r);
  } else {
    d = ea_read_byte(cpu, op&0x3f, 0);
    ADD_CYCLE(4);
    r = (cpu->d[reg]&0xff)-d;
    cpu_set_flags_sub(cpu, d&0x80, cpu->d[reg]&0x80, r&0x80, r);
    cpu->d[reg] = r;
  }
}

static void sub_w(struct cpu *cpu, WORD op)
{
  int rsrc,reg;
  WORD d,r;

  rsrc = op&0x100;
  reg = (op&0xe00)>>9;

  if(rsrc) {
    d = ea_read_word(cpu, op&0x3f, 1);
    ADD_CYCLE(8);
    r = d-(cpu->d[reg]&0xffff);
    ea_write_word(cpu, op&0x3f, r);
    cpu_set_flags_sub(cpu, cpu->d[reg]&0x8000, d&0x8000, r&0x8000, r);
  } else {
    d = ea_read_word(cpu, op&0x3f, 0);
    ADD_CYCLE(4);
    r = (cpu->d[reg]&0xffff)-d;
    cpu_set_flags_sub(cpu, d&0x8000, cpu->d[reg]&0x8000, r&0x8000, r);
    cpu->d[reg] = r;
  }
}

static void sub_l(struct cpu *cpu, WORD op)
{
  int rsrc,reg;
  LONG d,r;

  rsrc = op&0x100;
  reg = (op&0xe00)>>9;

  if(rsrc) {
    d = ea_read_long(cpu, op&0x3f, 1);
    ADD_CYCLE(12);
    r = d-cpu->d[reg];
    ea_write_long(cpu, op&0x3f, r);
    cpu_set_flags_sub(cpu, cpu->d[reg]&0x80000000, d&0x80000000, r&0x80000000, r);
  } else {
    d = ea_read_long(cpu, op&0x3f, 0);
    ADD_CYCLE(6);
    r = cpu->d[reg]-d;
    cpu_set_flags_sub(cpu, d&0x80000000, cpu->d[reg]&0x80000000, r&0x80000000, r);
    cpu->d[reg] = r;
  }
}

static void sub(struct cpu *cpu, WORD op)
{
  ENTER;

  switch((op&0xc0)>>6) {
  case 0:
    sub_b(cpu, op);
    return;
  case 1:
    sub_w(cpu, op);
    return;
  case 2:
    sub_l(cpu, op);
    return;
  }
}

static struct cprint *sub_print(LONG addr, WORD op)
{
  int s,r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  s = (op&0xc0)>>6;
  r = (op&0xe00)>>9;

  switch(s) {
  case 0:
    strcpy(ret->instr, "SUB.B");
    break;
  case 1:
    strcpy(ret->instr, "SUB.W");
    break;
  case 2:
    strcpy(ret->instr, "SUB.L");
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

void sub_init(void *instr[], void *print[])
{
  int i,r;

  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      instr[0x9000|(r<<9)|i] = sub;
      instr[0x9100|(r<<9)|i] = sub;
      instr[0x9040|(r<<9)|i] = sub;
      instr[0x9140|(r<<9)|i] = sub;
      instr[0x9080|(r<<9)|i] = sub;
      instr[0x9180|(r<<9)|i] = sub;
      print[0x9000|(r<<9)|i] = sub_print;
      print[0x9100|(r<<9)|i] = sub_print;
      print[0x9040|(r<<9)|i] = sub_print;
      print[0x9140|(r<<9)|i] = sub_print;
      print[0x9080|(r<<9)|i] = sub_print;
      print[0x9180|(r<<9)|i] = sub_print;
    }
  }
}
