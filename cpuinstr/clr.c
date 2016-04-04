#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

#define CLR_PREFETCH  1
#define CLR_WRITE     2

static void clr(struct cpu *cpu, WORD op)
{
  ENTER;

  switch(cpu->instr_state) {
  case INSTR_STATE_NONE:
    switch((op&0xc0)>>6) {
    case 0: ea_read_byte(cpu, op&0x3f, 1); break;
    case 1: ea_read_word(cpu, op&0x3f, 1); break;
    case 2: ea_read_long(cpu, op&0x3f, 1); break;
    }
    cpu->instr_state = CLR_PREFETCH;
    break;
  case CLR_PREFETCH:
    ADD_CYCLE(4);
    cpu->instr_state = CLR_WRITE;
    break;
  case CLR_WRITE:
    switch((op&0xc0)>>6) {
    case 0: ea_write_byte(cpu, op&0x3f, 0); break;
    case 1: ea_write_word(cpu, op&0x3f, 0); break;
    case 2:
      ea_write_long(cpu, op&0x3f, 0);
      // Clearing a data register takes two additional cycles.
      if(((op&0x38)>>3) == 0)
	ADD_CYCLE(2);
      break;
    }
    cpu->instr_state = INSTR_STATE_FINISHED;
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

