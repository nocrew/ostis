#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

static void clr(struct cpu *cpu, WORD op)
{
  int r;

  ENTER;

  r = 0;
  if(!(op&0x38)) r = 1;

  switch((op&0xc0)>>6) {
  case 0:
    ADD_CYCLE(4);
    if(!r) {
      ADD_CYCLE(4);
    }
    ea_write_byte(cpu, op&0x3f, 0);
    break;
  case 1:
    ADD_CYCLE(4);
    if(!r) {
      ADD_CYCLE(4);
    }
    ea_write_word(cpu, op&0x3f, 0);
    break;
  case 2:
    ADD_CYCLE(6);
    if(!r) {
      ADD_CYCLE(6);
    }
    ea_write_long(cpu, op&0x3f, 0);
    break;
  }
  cpu_set_flags_clr(cpu);
}

static struct cprint *clr_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);
  
  switch((op&0xc0)>>6) {
  case 0:
    strcpy(ret->instr, "CLR.B");
    break;
  case 1:
    strcpy(ret->instr, "CLR.W");
    break;
  case 2:
    strcpy(ret->instr, "CLR.L");
    break;
  }
  ea_print(ret, op&0x3f, 0);

  return ret;
}

void clr_init(void *instr[], void *print[])
{
  int i;
  
  for(i=0;i<0x40;i++) {
    if(ea_valid(i, EA_INVALID_DST|EA_INVALID_A)) {
      instr[0x4200|i] = clr;
      instr[0x4240|i] = clr;
      instr[0x4280|i] = clr;
      print[0x4200|i] = clr_print;
      print[0x4240|i] = clr_print;
      print[0x4280|i] = clr_print;
    }
  }
}

