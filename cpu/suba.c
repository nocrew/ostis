#include "common.h"
#include "cpu.h"
#include "mmu.h"
#include "ea.h"

static void suba(struct cpu *cpu, WORD op)
{
  LONG s;
  int r;

  ENTER;

  r = (op&0xe00)>>9;
  if(op&0x100) {
    if(((op&0x38) == 0) ||
       ((op&0x38) == 8) ||
       (((op&0x38) == 0x38) & ((op&7) == 4))) {
      ADD_CYCLE(8);
    } else {
      ADD_CYCLE(6);
    }
    s = ea_read_long(cpu, op&0x3f, 0);
  } else {
    ADD_CYCLE(8);
    s = ea_read_word(cpu, op&0x3f, 0);
    if(s&0x8000) s |= 0xffff0000;
  }
  cpu->a[r] = cpu->a[r]-s;
}

static struct cprint *suba_print(LONG addr, WORD op)
{
  int r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  r = (op&0xe00)>>9;

  if(op&0x100) {
    strcpy(ret->instr, "SUBA.L");
    ea_print(ret, op&0x3f, 2);
  } else {
    strcpy(ret->instr, "SUBA.W");
    ea_print(ret, op&0x3f, 1);
  }
  sprintf(ret->data, "%s,A%d", ret->data, r);

  return ret;
}

void suba_init(void *instr[], void *print[])
{
  int i,r;
  
  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      if(ea_valid(i, EA_INVALID_NONE)) {
	instr[0x90c0|(r<<9)|i] = suba;
	instr[0x91c0|(r<<9)|i] = suba;
	print[0x90c0|(r<<9)|i] = suba_print;
	print[0x91c0|(r<<9)|i] = suba_print;
      }
    }
  }
}
