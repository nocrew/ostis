#include "common.h"
#include "cpu.h"

static void trap(struct cpu *cpu, WORD op)
{
  ENTER;

  cpu_set_exception(32+(op&0xf));
}

static struct cprint *trap_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, "TRAP");
  sprintf(ret->data, "#%d", op&0xf);

  return ret;
}

void trap_init(void *instr[], void *print[])
{
  int i;

  for(i=0;i<0x10;i++) {
    instr[0x4e40|i] = trap;
    print[0x4e40|i] = trap_print;
  }
}
