#include "common.h"
#include "cpu.h"
#include "mmu.h"

static void stop(struct cpu *cpu, WORD op)
{
  ENTER;

  if(cpu->sr&0x2000) {
    ADD_CYCLE(4);
    cpu_set_sr(mmu_read_word(cpu->pc));
    cpu->pc += 2;
    cpu->stopped = 1;
  } else {
    cpu_set_exception(8); /* Privilege violation */
  }
}

static struct cprint *stop_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, "STOP");
  sprintf(ret->data, "#$%04x", mmu_read_word_print(addr+ret->size));
  ret->size += 2;
  
  return ret;
}

void stop_init(void *instr[], void *print[])
{
  instr[0x4e72] = stop;
  print[0x4e72] = stop_print;
}
