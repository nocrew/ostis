#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"

static void andi_to_ccr(struct cpu *cpu, WORD op)
{
  WORD d;
  WORD sr;

  ENTER;

  ADD_CYCLE(20);
  d = bus_read_word(cpu->pc)&0x1f;
  cpu->pc += 2;
  sr = cpu->sr;
  sr = (sr&0xff00)|((sr&0xff)&d);
  cpu_set_sr(sr);
  cpu_prefetch();
}

static struct cprint *andi_to_ccr_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, "ANDI");
  sprintf(ret->data, "#$%x,CCR", bus_read_word_print(addr+ret->size)&0xff);
  ret->size += 2;
  
  return ret;
}

void andi_to_ccr_init(void *instr[], void *print[])
{
  instr[0x023c] = andi_to_ccr;
  print[0x023c] = andi_to_ccr_print;
}



