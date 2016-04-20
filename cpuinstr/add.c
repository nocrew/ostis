#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

#define ADD_READ      1
#define ADD_PREFETCH  2
#define ADD_WRITE     3

static int long_ops(WORD op)
{
  int mode = (op & 0x3f);
  int ops = 1;
  if(mode == 0x3c) {
    ops = 2;
  }
  mode &= 0x38;
  if(mode == 0x00 || mode == 0x08)
    ops = 2;
  return ops;
}

/*
 * ------------------------------------------------------------------
 *                |    Exec Time    |          Data Bus Usage
 *      ADD       |  INSTR     EA   | Memory Operand |   INSTR
 * ---------------+-----------------+----------------+---------------
 * <ea>,Dn :      |                 |                |             
 *   .B or .W :   |                 |                |             
 *     Dn         |  4(1/0)  0(0/0) |                | np          
 *     An         |  4(1/0)  0(0/0) |                | np          
 *     (An)       |  4(1/0)  4(1/0) |            nr  | np          
 *     (An)+      |  4(1/0)  4(1/0) |            nr  | np          
 *     -(An)      |  4(1/0)  6(1/0) | n          nr  | np          
 *     (d16,An)   |  4(1/0)  8(2/0) |      np    nr  | np          
 *     (d8,An,Xn) |  4(1/0) 10(2/0) | n    np    nr  | np          
 *     (xxx).W    |  4(1/0)  8(2/0) |      np    nr  | np          
 *     (xxx).L    |  4(1/0) 12(3/0) |   np np    nr  | np          
 *     #<data>    |  4(1/0)  4(1/0)        np        | np          
 *   .L :         |                 |                |             
 *     Dn         |  8(1/0)  0(0/0) |                | np       nn 
 *     An         |  8(1/0)  0(0/0) |                | np       nn 
 *     (An)       |  6(1/0)  8(2/0) |         nR nr  | np       n  
 *     (An)+      |  6(1/0)  8(2/0) |         nR nr  | np       n  
 *     -(An)      |  6(1/0) 10(2/0) | n       nR nr  | np       n  
 *     (d16,An)   |  6(1/0) 12(3/0) |      np nR nr  | np       n  
 *     (d8,An,Xn) |  6(1/0) 14(3/0) | n    np nR nr  | np       n  
 *     (xxx).W    |  6(1/0) 12(3/0) |      np nR nr  | np       n  
 *     (xxx).L    |  6(1/0) 16(4/0) |   np np nR nr  | np       n  
 *     #<data>    |  8(1/0)  8(2/0) |   np np        | np       nn 
 * Dn,<ea> :      |                 |                |             
 *   .B or .W :   |                 |                |             
 *     (An)       |  8(1/1)  4(1/0) |            nr  | np nw       
 *     (An)+      |  8(1/1)  4(1/0) |            nr  | np nw       
 *     -(An)      |  8(1/1)  6(1/0) | n          nr  | np nw       
 *     (d16,An)   |  8(1/1)  8(2/0) |      np    nr  | np nw       
 *     (d8,An,Xn) |  8(1/1) 10(2/0) | n    np    nr  | np nw       
 *     (xxx).W    |  8(1/1)  8(2/0) |      np    nr  | np nw       
 *     (xxx).L    |  8(1/1) 12(3/0) |   np np    nr  | np nw       
 *   .L :         |                 |                |             
 *     (An)       | 12(1/2)  8(2/0) |            nr  | np nw nW    
 *     (An)+      | 12(1/2)  8(2/0) |            nr  | np nw nW    
 *     -(An)      | 12(1/2) 10(2/0) | n       nR nr  | np nw nW    
 *     (d16,An)   | 12(1/2) 12(3/0) |      np nR nr  | np nw nW    
 *     (d8,An,Xn) | 12(1/2) 14(3/0) | n    np nR nr  | np nw nW    
 *     (xxx).W    | 12(1/2) 12(3/0) |      np nR nr  | np nw nW    
 *     (xxx).L    | 12(1/2) 16(4/0) |   np np nR nr  | np nw nW    
 */
static void add(struct cpu *cpu, WORD op)
{
  LONG operand, r, m = 0;
  int reg;
  ENTER;

  switch(cpu->instr_state) {
  case INSTR_STATE_NONE:
    ea_begin_read(cpu, op);
    cpu->instr_state = ADD_READ;
    // Fall through.
  case ADD_READ:
    if(!ea_done(&operand)) {
      ADD_CYCLE(2);
      break;
    } else {
      cpu->instr_state = ADD_PREFETCH;
    }
    // Fall through.
  case ADD_PREFETCH:
    ADD_CYCLE(4);
    cpu_prefetch();
    cpu->instr_state = ADD_WRITE;
    reg = (op&0xe00)>>9;
    r = operand + cpu->d[reg];

    switch((op>>6)& 3) {
    case 0: m = 0x80; r &= 0xff; break;
    case 1: m = 0x8000; r &= 0xffff; break;
    case 2: m = 0x80000000; break;
    }

    if(op & 0x100) {
      ea_begin_modify(cpu, op, r, 0, long_ops(op), 0, 0);
      cpu_set_flags_add(cpu, cpu->d[reg]&m, operand&m, r&m, r);
    } else {
      cpu_set_flags_add(cpu, operand&m, cpu->d[reg]&m, r&m, r);
      switch((op >> 6) & 3) {
      case 0:
	cpu->d[reg] = (cpu->d[reg] & 0xffffff00) + r;
	break;
      case 1:
	cpu->d[reg] = (cpu->d[reg] & 0xffff0000) + r;
	break;
      case 2:
	cpu->d[reg] = r;
	break;
      }
    }
    break;
  case ADD_WRITE:
    if(ea_done(&operand)) {
      cpu->instr_state = INSTR_STATE_FINISHED;
    } else {
      ADD_CYCLE(2);
    }
    break;
  }
}

static struct cprint *add_print(LONG addr, WORD op)
{
  int s,r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  s = (op&0xc0)>>6;
  r = (op&0xe00)>>9;

  switch(s) {
  case 0:
    strcpy(ret->instr, "ADD.B");
    break;
  case 1:
    strcpy(ret->instr, "ADD.W");
    break;
  case 2:
    strcpy(ret->instr, "ADD.L");
    break;
  }

  if(op&0x100) {
    sprintf(ret->data, "D%d,", r);
    ea_print(ret, op&0x3f, s);
  } else {
    ea_print(ret, op&0x3f, s);
    sprintf(ret->data, "%s,D%d", ret->data, r);
  }
  
  return ret;
}

void add_instr_init(void *instr[], void *print[])
{
  int i,r;

  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      if(ea_valid(i, EA_INVALID_A)) {
	instr[0xd000|(r<<9)|i] = add;
	print[0xd000|(r<<9)|i] = add_print;
      }
      if(ea_valid(i, EA_INVALID_NONE)) {
	instr[0xd040|(r<<9)|i] = add;
	instr[0xd080|(r<<9)|i] = add;
	print[0xd040|(r<<9)|i] = add_print;
	print[0xd080|(r<<9)|i] = add_print;
      }
      if(ea_valid(i, EA_INVALID_MEM)) {
	instr[0xd100|(r<<9)|i] = add;
	instr[0xd140|(r<<9)|i] = add;
	instr[0xd180|(r<<9)|i] = add;
	print[0xd100|(r<<9)|i] = add_print;
	print[0xd140|(r<<9)|i] = add_print;
	print[0xd180|(r<<9)|i] = add_print;
      }
    }
  }
}

