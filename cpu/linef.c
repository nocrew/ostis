#include "common.h"
#include "cpu.h"

static void linef(struct cpu *cpu, WORD op)
{
  ENTER;
  
  cpu->pc -= 2;
  cpu_set_exception(11);
}

static struct cprint *linef_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);
  
  strcpy(ret->instr, "DC.W");
  sprintf(ret->data, "$%04X", op);

  return ret;
}

void linef_init(void *instr[], void *print[])
{
  int i;

  for(i=0;i<0x1000;i++) {
    instr[0xf000|i] = linef;
    print[0xf000|i] = linef_print;
  }
}
