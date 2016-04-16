#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

#define NEGX_READ      1
#define NEGX_PREFETCH  2
#define NEGX_WRITE     3

static void negx(struct cpu *cpu, WORD op)
{
  LONG operand;
  ENTER;

  switch(cpu->instr_state) {
  case INSTR_STATE_NONE:
    ea_begin_read(cpu, op);
    cpu->instr_state = NEGX_READ;
    // Fall through.
  case NEGX_READ:
    if(ea_done(&operand))
      cpu->instr_state = NEGX_PREFETCH;
    else {
      ADD_CYCLE(2);
      break;
    }
    // Fall through.
  case NEGX_PREFETCH:
    ADD_CYCLE(4);
    cpu->instr_state = NEGX_WRITE;
    operand = -operand;
    if(CHKX) operand--;
    ea_begin_modify(cpu, op, operand, 0, 2, 0, 0);
    break;
  case NEGX_WRITE:
    if(ea_done(&operand)) {
      cpu->instr_state = INSTR_STATE_FINISHED;
    } else
      ADD_CYCLE(2);
    cpu_set_flags_clr(cpu);
    break;
  }
}

static struct cprint *negx_print(LONG addr, WORD op)
{
  int s;

  struct cprint *ret;

  ret = cprint_alloc(addr);
  
  s = (op&0xc0)>>6;

  switch(s) {
  case 0:
    strcpy(ret->instr, "NEGX.B");
    break;
  case 1:
    strcpy(ret->instr, "NEGX.W");
    break;
  case 2:
    strcpy(ret->instr, "NEGX.L");
    break;
  }
  
  ea_print(ret, op&0x3f, s);

  return ret;
}

void negx_instr_init(void *instr[], void *print[])
{
  int i;

  for(i=0;i<0x40;i++) {
    if(ea_valid(i, EA_INVALID_A|EA_INVALID_DST)) {
      instr[0x4000|i] = negx;
      instr[0x4040|i] = negx;
      instr[0x4080|i] = negx;
      print[0x4000|i] = negx_print;
      print[0x4040|i] = negx_print;
      print[0x4080|i] = negx_print;
    }
  }
}
