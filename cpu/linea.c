#include "common.h"
#include "cpu.h"

static void linea(struct cpu *cpu, WORD op)
{
  ENTER;
  
  cpu->pc -= 2;
  cpu_set_exception(10);
}

static struct cprint *linea_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);
  
  strcpy(ret->instr, "DC.W");
  sprintf(ret->data, "$%04X", op);

  return ret;
}

void linea_init(void *instr[], void *print[])
{
  int i;

  for(i=0;i<0x1000;i++) {
    instr[0xa000|i] = linea;
    print[0xa000|i] = linea_print;
  }
}
