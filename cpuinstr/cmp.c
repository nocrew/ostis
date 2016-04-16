#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"
#include "ea.h"

#define CMP_READ      1
#define CMP_PREFETCH  2
#define CMP_LONG      3

static void cmp(struct cpu *cpu, WORD op)
{
  LONG s=0,d,r,m=0;

  ENTER;

  switch(cpu->instr_state) {
  case INSTR_STATE_NONE:
    ea_begin_read(cpu, op);
    cpu->instr_state = CMP_READ;
    // Fall through.
  case CMP_READ:
    if(ea_done(&s)) {
      d = cpu->d[(op&0xe00)>>9];
      r = d-s;
      switch((op&0xc0)>>6) {
      case 0: m = 0x80; r &= 0xff; break;
      case 1: m = 0x8000; r &= 0xffff; break;
      case 2: m = 0x80000000; break;
      }
      cpu_set_flags_cmp(cpu, s&m, d&m, r&m, r);
      cpu->instr_state = CMP_PREFETCH;
    } else {
      ADD_CYCLE(2);
      break;
    }
    // Fall through.
  case CMP_PREFETCH:
    ADD_CYCLE(4);
    if(((op&0xc0)>>6) == 2)
      cpu->instr_state = CMP_LONG;
    else
      cpu->instr_state = INSTR_STATE_FINISHED;
    break;
  case CMP_LONG:
    ADD_CYCLE(2);
    cpu->instr_state = INSTR_STATE_FINISHED;
    break;
  }
}

static struct cprint *cmp_print(LONG addr, WORD op)
{
  int r;
  int s;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  r = (op&0xe00)>>9;
  s = (op&0xc0)>>6;

  switch(s) {
  case 0:
    strcpy(ret->instr, "CMP.B");
    break;
  case 1:
    strcpy(ret->instr, "CMP.W");
    break;
  case 2:
    strcpy(ret->instr, "CMP.L");
    break;
  }
  ea_print(ret, op&0x3f, s);
  sprintf(ret->data, "%s,D%d", ret->data, r);

  return ret;
}

void cmp_instr_init(void *instr[], void *print[])
{
  int r,i;

  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      if(ea_valid(i, EA_INVALID_A)) {
	instr[0xb000|(r<<9)|i] = cmp;
	print[0xb000|(r<<9)|i] = cmp_print;
      }
      if(ea_valid(i, EA_INVALID_NONE)) {
	instr[0xb040|(r<<9)|i] = cmp;
	instr[0xb080|(r<<9)|i] = cmp;
	print[0xb040|(r<<9)|i] = cmp_print;
	print[0xb080|(r<<9)|i] = cmp_print;
      }
    }
  }
}
