#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"

static void rts(struct cpu *cpu, WORD op)
{
  ENTER;

  cpu->pc = mmu_read_long(cpu->a[7]);
  cpu->a[7] += 4;
  ADD_CYCLE(16);
}

static struct cprint *rts_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, "RTS");
  
  return ret;
}

void rts_init(void *instr[], void *print[])
{
  instr[0x4e75] = rts;
  print[0x4e75] = rts_print;
}
