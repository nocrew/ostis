#include "common.h"
#include "cpu.h"
#include "ea.h"
#include "mmu.h"

static void pea(struct cpu *cpu, WORD op)
{
  LONG a;

  a = ea_get_addr(cpu, op&0x3f);
  cpu->a[7] -= 4;
  mmu_write_long(cpu->a[7], a);
  ADD_CYCLE(8);
}

static struct cprint *pea_print(LONG addr, WORD op)
{
  struct cprint *ret;
  
  ret = cprint_alloc(addr);

  strcpy(ret->instr, "PEA");
  ea_print(ret, op&0x3f, 0);

  return ret;
}

void pea_init(void *instr[], void *print[])
{
  int i;

  for(i=0;i<0x40;i++) {
    instr[0x4840|i] = pea;
    print[0x4840|i] = pea_print;
  }
}
