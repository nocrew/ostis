#include "common.h"
#include "cpu.h"
#include "mmu.h"

static void rtr(struct cpu *cpu, WORD op)
{
  WORD ccr;

  ENTER;

  ADD_CYCLE(20);
  ccr = mmu_read_word(cpu->a[7]);
  cpu->a[7] += 2;
  cpu->pc = mmu_read_long(cpu->a[7]);
  cpu->a[7] += 4;
  cpu_set_sr((cpu->sr&0xffe0)|(ccr|0x1f));
}

static struct cprint *rtr_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);
  
  strcpy(ret->instr, "RTR");

  return ret;
}

void rtr_init(void *instr[], void *print[])
{
  instr[0x4e77] = rtr;
  print[0x4e77] = rtr_print;
}
