#include "common.h"
#include "cpu.h"
#include "mmu.h"

static void unlk(struct cpu *cpu, WORD op)
{
  int r;

  ENTER;

  r = op&0x7;

  cpu->a[7] = cpu->a[r];
  cpu->a[r] = mmu_read_long(cpu->a[7]);
  cpu->a[7] += 4;

  ADD_CYCLE(12);
}

static struct cprint *unlk_print(LONG addr, WORD op)
{
  int r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  r = op&0x7;

  strcpy(ret->instr, "UNLK");
  sprintf(ret->data, "A%d", r);

  return ret;
}

void unlk_init(void *instr[], void *print[])
{
  int r;
  for(r=0;r<8;r++) {
    instr[0x4e58|r] = unlk;
    print[0x4e58|r] = unlk_print;
  }
}
