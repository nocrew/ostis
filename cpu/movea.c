#include "common.h"
#include "cpu.h"
#include "ea.h"

static void movea(struct cpu *cpu, WORD op)
{
  LONG s;

  ENTER;

  ADD_CYCLE(4);

  switch((op&0x1000)>>12) {
  case 0:
    s = ea_read_long(cpu, op&0x3f, 0);
    break;
  case 1:
    s = ea_read_word(cpu, op&0x3f, 0);
    if(s&0x8000) s |= 0xffff0000;
    break;
  default:
    s = 0;
  }

  cpu->a[(op&0xe00)>>9] = s;
}

static struct cprint *movea_print(LONG addr, WORD op)
{
  int r,s;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  r = (op&0xe00)>>9;
  s = (op&0x1000)>>12;
  if(s==0) s = 2;

  switch(s) {
  case 1:
    strcpy(ret->instr, "MOVEA.W");
    break;
  case 2:
    strcpy(ret->instr, "MOVEA.L");
    break;
  }
  ea_print(ret, op&0x3f, s);
  sprintf(ret->data, "%s,A%d", ret->data, r);

  return ret;
}

void movea_init(void *instr[], void *print[])
{
  int i,r;
  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      if(ea_valid(i, EA_INVALID_NONE)) {
	instr[0x2040|(r<<9)|i] = movea;
	instr[0x3040|(r<<9)|i] = movea;
	print[0x2040|(r<<9)|i] = movea_print;
	print[0x3040|(r<<9)|i] = movea_print;
      }
    }
  }
}
