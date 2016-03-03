#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

static void move_to_ccr(struct cpu *cpu, WORD op)
{
  WORD sr;

  ENTER;

  ADD_CYCLE(12);
  sr = ea_read_word(cpu, op&0x3f, 0);
  cpu_set_sr((cpu->sr&0xffe0)|(sr&0x1f));
}

static struct cprint *move_to_ccr_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, "MOVE");
  ea_print(ret, op&0x3f, 1);
  strcat(ret->data, ",CCR");

  return ret;
}

void move_to_ccr_init(void *instr[], void *print[])
{
  int i;
  for(i=0;i<0x40;i++) {
    if(ea_valid(i, EA_INVALID_A)) {
      instr[0x44c0|i] = (void *)move_to_ccr;
      print[0x44c0|i] = (void *)move_to_ccr_print;
    }
  }
}
