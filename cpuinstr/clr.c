#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

#define CLR_READ      1
#define CLR_PREFETCH  2
#define CLR_WRITE     3

static void clr(struct cpu *cpu, WORD op)
{
  LONG operand;
  ENTER;

  switch(cpu->instr_state) {
  case INSTR_STATE_NONE:
    ea_begin_read(cpu, op);
    cpu->instr_state = CLR_READ;
    // Fall through.
  case CLR_READ:
    if(ea_done(&operand))
      cpu->instr_state = CLR_PREFETCH;
    else {
      ADD_CYCLE(2);
      break;
    }
    // Fall through.
  case CLR_PREFETCH:
    ADD_CYCLE(4);
    cpu->instr_state = CLR_WRITE;
    ea_begin_modify(cpu, op, 0, 0, 2, 0, 0);
    break;
  case CLR_WRITE:
    if(ea_done(&operand)) {
      cpu->instr_state = INSTR_STATE_FINISHED;
    } else
      ADD_CYCLE(2);
    cpu_set_flags_clr(cpu);
    break;
  }
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

void clr_instr_init(void *instr[], void *print[])
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

