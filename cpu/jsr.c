#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"
#include "ea.h"

static void jsr(struct cpu *cpu, WORD op)
{
  LONG newpc;
  
  ENTER;

  ADD_CYCLE(12);
  cpu->cyclecomp = 1;
  newpc = ea_get_addr(cpu, op&0x3f);
  cpu->a[7] -= 4;
  mmu_write_long(cpu->a[7], cpu->pc);
  cpu->pc = newpc;
  cprint_set_label(cpu->pc, NULL);
}

static struct cprint *jsr_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, "JSR");
  ea_print(ret, op&0x3f, 0);
  
  return ret;
}

void jsr_init(void *instr[], void *print[])
{
  int i;
  
  for(i=0;i<0x40;i++) {
    if(ea_valid(i, EA_INVALID_EA)) {
      instr[0x4e80|i] = jsr;
      print[0x4e80|i] = jsr_print;
    }
  }
}
