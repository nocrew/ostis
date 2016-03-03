#include "common.h"
#include "cpu.h"
#include "cprint.h"

static void reset(struct cpu *cpu, WORD op)
{
  ENTER;

  ADD_CYCLE(128);
}

static struct cprint *reset_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, "RESET");

  return ret;
}

void reset_init(void *instr[], void *print[])
{
  instr[0x4e70] = reset;
  print[0x4e70] = reset_print;
}
