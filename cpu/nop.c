#include "common.h"
#include "cprint.h"

static void nop(struct cpu *cpu, WORD op)
{
  ENTER;

  ADD_CYCLE(4);
  cpu_prefetch();
}

static struct cprint *nop_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, "NOP");
  
  return ret;
}

void nop_init(void *instr[], void *print[])
{
  instr[0x4e71] = nop;
  print[0x4e71] = nop_print;
}
