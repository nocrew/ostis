#include "common.h"
#include "cpu.h"
#include "cprint.h"

static void swap(struct cpu *cpu, WORD op)
{
  int r;

  r = op&0x7;

  cpu->d[r] = ((cpu->d[r]&0xffff0000)>>16)|((cpu->d[r]&0xffff)<<16);
  ADD_CYCLE(4);
  cpu_set_flags_move(cpu, cpu->d[r]&0x80000000, cpu->d[r]);
  cpu_prefetch();
}

static struct cprint *swap_print(LONG addr, WORD op)
{
  int r;
  struct cprint *ret;

  ret = cprint_alloc(addr);
  
  r = op&0x7;

  strcpy(ret->instr, "SWAP");
  sprintf(ret->data, "D%d", r);

  return ret;
}

void swap_init(void *instr[], void *print[])
{
  int r;

  for(r=0;r<8;r++) {
    instr[0x4840|r] = swap;
    print[0x4840|r] = swap_print;
  }
}
