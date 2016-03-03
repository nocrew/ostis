#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"
#include "ea.h"

static void lea(struct cpu *cpu, WORD op)
{
  int r;

  ENTER;

  r = (op&0xe00)>>9;
  cpu->a[r] = ea_get_addr(cpu, op&0x3f);
  // x(An,Dn) and x(PC,Dn) needs an extra 4 cycles due to internal workings of LEA and alignments in the ST
  if((op&0x38) == 0x30 || (op&0x3f) == 0x3b) {
    ADD_CYCLE(4);
  }
}

static struct cprint *lea_print(LONG addr, WORD op)
{
  struct cprint *ret;
  
  ret = cprint_alloc(addr);

  strcpy(ret->instr, "LEA");
  ea_print(ret, op&0x3f, 0);
  sprintf(ret->data, "%s,A%d", ret->data, (op&0xe00)>>9);

  return ret;
}

void lea_init(void *instr[], void *print[])
{
  int i,r;

  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      if(ea_valid(i, EA_INVALID_EA)) {
	instr[0x41c0|(r<<9)|i] = lea;
	print[0x41c0|(r<<9)|i] = lea_print;
      }
    }
  }
}
