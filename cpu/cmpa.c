#include "common.h"
#include "cpu.h"
#include "ea.h"

static void cmpa(struct cpu *cpu, WORD op)
{
  LONG s,d;
  LONG r;
  
  ENTER;

  ADD_CYCLE(6);

  s = cpu->a[(op&0xe00)>>9];
  if(op&0x100) {
    d = ea_read_long(cpu, op&0x3f, 0);
  } else {
    d = ea_read_word(cpu, op&0x3f, 0);
    if(d&0x8000) d |= 0xffff0000;
  }
  r = d-s;
  cpu_set_flags_sub(cpu, s&0x80000000, d&0x80000000, r&0x80000000, r);
}

static struct cprint *cmpa_print(LONG addr, WORD op)
{
  int r,s;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  r = (op&0xe00)>>9;
  s = (op&0x100)>>8;

  switch(s) {
  case 0:
    strcpy(ret->instr, "CMPA.W");
    break;
  case 1:
    strcpy(ret->instr, "CMPA.L");
    break;
  }
  ea_print(ret, op&0x3f, s+1);
  sprintf(ret->data, "%s,A%d", ret->data, r);

  return ret;
}

void cmpa_init(void *instr[], void *print[])
{
  int i,r;
  
  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      instr[0xb0c0|(r<<9)|i] = cmpa;
      instr[0xb1c0|(r<<9)|i] = cmpa;
      print[0xb0c0|(r<<9)|i] = cmpa_print;
      print[0xb1c0|(r<<9)|i] = cmpa_print;
    }
  }
}
