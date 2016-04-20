#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"

static void eori_to_sr(struct cpu *cpu, WORD op)
{
  WORD d;

  ENTER;

  if(cpu->sr&0x2000) {
    ADD_CYCLE(20);
    d = bus_read_word(cpu->pc);
    cpu->pc += 2;
    cpu_set_sr(cpu->sr^d);
    cpu_prefetch();
  } else {
    cpu_set_exception(8); /* Privilege violation */
  }
}

static struct cprint *eori_to_sr_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, "EORI");
  sprintf(ret->data, "#$%x,SR", bus_read_word_print(addr+ret->size));
  ret->size += 2;
  
  return ret;
}

void eori_to_sr_init(void *instr[], void *print[])
{
  instr[0x0a7c] = eori_to_sr;
  print[0x0a7c] = eori_to_sr_print;
}
