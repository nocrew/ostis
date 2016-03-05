#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

static void move_from_sr(struct cpu *cpu, WORD op)
{
  ENTER;

  ADD_CYCLE(8); /* really 6 for EA==reg, but that will be 8 anyway */
  ea_set_prefetch_before_write();
  ea_write_word(cpu, op&0x3f, cpu->sr);
}

static struct cprint *move_from_sr_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, "MOVE");
  strcpy(ret->data, "SR,");
  ea_print(ret, op&0x3f, 1);

  return ret;
}

void move_from_sr_init(void *instr[], void *print[])
{
  int i;
  for(i=0;i<0x40;i++) {
    if(ea_valid(i, EA_INVALID_DST|EA_INVALID_A)) {
      instr[0x40c0|i] = (void *)move_from_sr;
      print[0x40c0|i] = (void *)move_from_sr_print;
    }
  }
}
