#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"

static void ori_to_sr(struct cpu *cpu, WORD op)
{
  WORD d;

  ENTER;

  if(cpu->sr&0x2000) {
    ADD_CYCLE(20);
    d = bus_read_word(cpu->pc);
    cpu->pc += 2;
    cpu_set_sr(cpu->sr|d);
  } else {
    cpu_set_exception(8); /* Privilege violation */
  }
}

static struct cprint *ori_to_sr_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, "ORI");
  sprintf(ret->data, "#$%x,SR", bus_read_word_print(addr+ret->size));
  ret->size += 2;
  
  return ret;
}

void ori_to_sr_init(void *instr[], void *print[])
{
  instr[0x007c] = ori_to_sr;
  print[0x007c] = ori_to_sr_print;
}
