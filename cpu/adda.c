#include "common.h"
#include "cpu.h"
#include "ea.h"

static void adda(struct cpu *cpu, WORD op)
{
  LONG s;
  int r;

  ENTER;

  r = (op&0xe00)>>9;

  if(op&0x100) {
    ADD_CYCLE(6);
    s = ea_read_long(cpu, op&0x3f, 0);
  } else {
    ADD_CYCLE(8);
    s = ea_read_word(cpu, op&0x3f, 0);
    if(s&0x8000) s |= 0xffff0000;
  }

  cpu->a[r] += s;
}

static struct cprint *adda_print(LONG addr, WORD op)
{
  int r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  r = (op&0xe00)>>9;

  if(op&0x100) {
    strcpy(ret->instr, "ADDA.L");
    ea_print(ret, op&0x3f, 2);
    sprintf(ret->data, "%s,A%d", ret->data, r);
  } else {
    strcpy(ret->instr, "ADDA.L");
    ea_print(ret, op&0x3f, 1);
    sprintf(ret->data, "%s,A%d", ret->data, r);
  }
  
  return ret;
}

void adda_init(void *instr[], void *print[])
{
  int i,r;
    
  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      if(ea_valid(i, EA_INVALID_NONE)) {
	instr[0xd0c0|(r<<9)|i] = adda;
	instr[0xd1c0|(r<<9)|i] = adda;
	print[0xd0c0|(r<<9)|i] = adda_print;
	print[0xd1c0|(r<<9)|i] = adda_print;
      }
    }
  }
}
