#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"

#define EXTREME_EXCEPTION_DEBUG 0

static void rte(struct cpu *cpu, WORD op)
{
  WORD sr;
  LONG pc;

  ENTER;

  if(cpu->sr&0x2000) {
    ADD_CYCLE(20);
    sr = bus_read_word(cpu->a[7]);
    cpu->a[7] += 2;
    pc = bus_read_long(cpu->a[7]);
    cpu->a[7] += 4;
#if EXTREME_EXCEPTION_DEBUG
    printf("DEBUG: %d Leaving interrupt [%08x / %04x => %08x / %04x]\n", cpu->cycle, cpu->pc, cpu->sr, pc, sr);
#endif
    cpu->pc = pc;
    cpu_set_sr(sr);
    cpu_prefetch();
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
