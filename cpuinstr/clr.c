#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"
#include "ucode.h"

/*
 * --------------------------------------------------------------------
 *                   |  Exec Time      |         Data Bus Usage
 *         CLR       |  INSTR     EA   |  1st OP (ea)  | INSTR
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

static void clr_compute(struct cpu *cpu, WORD op)
{
  ea_set_data(0);
  cpu_set_flags_clr(cpu);

  if((op & 0xc38) == 0x800) {
    ujump(nop_uops, 1);
  } else {
    ujump(0, 0);
  }
}

static u_sequence clr_seq[] =
{
  ea_begin_read,
  u_prefetch,
  clr_compute,
  ea_begin_modify,
  u_end_sequence
};

static void clr(struct cpu *cpu, WORD op)
{
  u_start_sequence(clr_seq, cpu, op);
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

