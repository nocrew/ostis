#include "common.h"
#include "cpu.h"
#include "mmu.h"

static void rte(struct cpu *cpu, WORD op)
{
  WORD sr;

  ENTER;

  if(cpu->sr&0x2000) {
    ADD_CYCLE(20);
    sr = mmu_read_word(cpu->a[7]);
    cpu->a[7] += 2;
    cpu->pc = mmu_read_long(cpu->a[7]);
    cpu->a[7] += 4;
    cpu_set_sr(sr);
    cpu->tracedelay = 1;
  } else {
    cpu_set_exception(8); /* Privilege violation */
  }
}

static struct cprint *rte_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);
  
  strcpy(ret->instr, "RTE");

  return ret;
}

void rte_init(void *instr[], void *print[])
{
  instr[0x4e73] = rte;
  print[0x4e73] = rte_print;
}
