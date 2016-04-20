#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

#define NEG_READ      1
#define NEG_PREFETCH  2
#define NEG_WRITE     3

/*
 * --------------------------------------------------------------------
 *                   |  Exec Time      |         Data Bus Usage
 *         NEG       |  INSTR     EA   |  1st OP (ea)  | INSTR
 * ------------------+-----------------+-------------------------------
 * <ea> :            |                 |               |
 *   .B or .W :      |                 |               |
 *     Dn            |  4(1/0)  0(0/0) |               | np
 *     (An)          |  8(1/1)  4(1/0) |            nr | np nw
 *     (An)+         |  8(1/1)  4(1/0) |            nr | np nw
 *     -(An)         |  8(1/1)  6(1/0) | n          nr | np nw
 *     (d16,An)      |  8(1/1)  8(2/0) |      np    nr | np nw
 *     (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr | np nw
 *     (xxx).W       |  8(1/1)  8(2/0) |      np    nr | np nw
 *     (xxx).L       |  8(1/1) 12(2/0) |   np np    nr | np nw
 *   .L :            |                 |               |
 *     Dn            |  6(1/0)  0(0/0) |               | np       n
 *     (An)          | 12(1/2)  8(2/0) |         nR nr | np nw nW
 *     (An)+         | 12(1/2)  8(2/0) |         nR nr | np nw nW
 *     -(An)         | 12(1/2) 10(2/0) | n       nR nr | np nw nW
 *     (d16,An)      | 12(1/2) 12(3/0) |      np nR nr | np nw nW
 *     (d8,An,Xn)    | 12(1/2) 14(3/0) | n    np nR nr | np nw nW
 *     (xxx).W       | 12(1/2) 12(3/0) |      np nR nr | np nw nW
 *     (xxx).L       | 12(1/2) 16(4/0) |   np np nR nr | np nw nW
 */
static void neg(struct cpu *cpu, WORD op)
{
  LONG operand;
  ENTER;

  switch(cpu->instr_state) {
  case INSTR_STATE_NONE:
    ea_begin_read(cpu, op);
    cpu->instr_state = NEG_READ;
    // Fall through.
  case NEG_READ:
    if(!ea_done(&operand)) {
      ADD_CYCLE(2);
      break;
    } else {
      cpu->instr_state = NEG_PREFETCH;
    }
    // Fall through.
  case NEG_PREFETCH:
    ADD_CYCLE(4);
    cpu_prefetch();
    cpu->instr_state = NEG_WRITE;
    ea_begin_modify(cpu, op, -operand, 0, 2, 0, 0);
    break;
  case NEG_WRITE:
    if(ea_done(&operand)) {
      cpu->instr_state = INSTR_STATE_FINISHED;
    } else {
      ADD_CYCLE(2);
    }
    cpu_set_flags_clr(cpu);
    break;
  }
}

static struct cprint *neg_print(LONG addr, WORD op)
{
  int s;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  s = (op&0xc0)>>6;
  
  switch(s) {
  case 0:
    strcpy(ret->instr, "NEG.B");
    break;
  case 1:
    strcpy(ret->instr, "NEG.W");
    break;
  case 2:
    strcpy(ret->instr, "NEG.L");
    break;
  }

  ea_print(ret, op&0x3f, s);

  return ret;
}

void neg_instr_init(void *instr[], void *print[])
{
  int i;
  
  for(i=0;i<0xc0;i++) {
    if(ea_valid(i, EA_INVALID_DST|EA_INVALID_A)) {
      instr[0x4400|i] = neg;
      print[0x4400|i] = neg_print;
    }
  }
}
