#include "common.h"
#include "cpu.h"
#include "ea.h"

static void move_to_sr(struct cpu *cpu, WORD op)
{
  ENTER;

  if(cpu->sr&0x2000) {
    ADD_CYCLE(12);
    cpu_set_sr(ea_read_word(cpu, op&0x3f, 0));
  } else {
    cpu_set_exception(8); /* Privilege violation */
  }
}

static struct cprint *move_to_sr_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, "MOVE");
  ea_print(ret, op&0x3f, 1);
  sprintf(ret->data, "%s,SR", ret->data);

  return ret;
}

void move_to_sr_init(void *instr[], void *print[])
{
  int i;
  for(i=0;i<0x40;i++) {
    instr[0x46c0|i] = (void *)move_to_sr;
    print[0x46c0|i] = (void *)move_to_sr_print;
  }
}
