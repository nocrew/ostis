#include "common.h"
#include "cpu.h"
#include "ea.h"

static void jmp(struct cpu *cpu, WORD op)
{
  ENTER;

  ADD_CYCLE(4);
  cpu->cyclecomp = 1;
  cpu->pc = ea_get_addr(cpu, op&0x3f);
  cprint_set_label(cpu->pc, NULL);
}

static struct cprint *jmp_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, "JMP");
  ea_print(ret, op&0x3f, 0);
  
  return ret;
}

void jmp_init(void *instr[], void *print[])
{
  int i;
  
  for(i=0;i<0x40;i++) {
    instr[0x4ec0|i] = jmp;
    print[0x4ec0|i] = jmp_print;
  }
}
