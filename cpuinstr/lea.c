#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"
#include "ea.h"

#define LEA_EA        1
#define LEA_PREFETCH  2

/*
 * ---------------------------------------------------
 *                   |  Exec Time  |  Data Bus Usage
 *         LEA       |    INSTR    |     INSTR
 * ------------------+-------------+------------------
 * <ea>,An :         |             |
 *   .L :            |             |
 *     (An)          |  4(1/0)     |          np
 *     (d16,An)      |  8(2/0)     |     np   np
 *     (d8,An,Xn)    | 12(2/0)     |   n np n np
 *     (xxx).W       |  8(2/0)     |     np   np
 *     (xxx).L       | 12(3/0)     |  np np   np
 */
static void lea(struct cpu *cpu, WORD op)
{
  ENTER;
  LONG operand;

  switch(cpu->instr_state) {
  case INSTR_STATE_NONE:
    ea_begin_address(cpu, op);
    cpu->instr_state = LEA_EA;
    // Fall through.
  case LEA_EA:
    if(ea_done(&operand)) {
      cpu->a[(op&0xe00)>>9] = ea_get_address();
      cpu->instr_state = LEA_PREFETCH;
    } else {
      ADD_CYCLE(2);
      break;
    }
    // Fall through.
  case LEA_PREFETCH:
    ADD_CYCLE(4);
    cpu_prefetch();
    cpu->instr_state = INSTR_STATE_FINISHED;
    break;
  }
}

static struct cprint *lea_print(LONG addr, WORD op)
{
  struct cprint *ret;
  
  ret = cprint_alloc(addr);

  strcpy(ret->instr, "LEA");
  ea_print(ret, op&0x3f, 0);
  sprintf(ret->data, "%s,A%d", ret->data, (op&0xe00)>>9);

  return ret;
}

void lea_instr_init(void *instr[], void *print[])
{
  int i,r;

  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      if(ea_valid(i, EA_INVALID_EA)) {
	instr[0x41c0|(r<<9)|i] = lea;
	print[0x41c0|(r<<9)|i] = lea_print;
      }
    }
  }
}
