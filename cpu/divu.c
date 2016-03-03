#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

static void divu(struct cpu *cpu, WORD op)
{
  LONG s,d,r;
  LONG m;
  int reg;

  ENTER;

  s = ea_read_word(cpu, op&0x3f, 0);
  reg = (op&0xe00)>>9;
  d = cpu->d[reg];
  if(!s) cpu_set_exception(5);
  r = d/s;
  m = d%s;
  cpu_set_flags_divu(cpu, r&0x80000000, r, r&0xffff0000);
  if(!(r&0xffff0000)) {
    r = ((m&0xffff)<<16)|(r&0xffff);
    cpu->d[reg] = r;
  }
  ADD_CYCLE(136);
}

static struct cprint *divu_print(LONG addr, WORD op)
{
  int r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  r = (op&0xe00)>>9;

  strcpy(ret->instr, "DIVU");
  ea_print(ret, op&0x3f, 1);
  sprintf(ret->data, "%s,D%d", ret->data, r);

  return ret;
}

void divu_init(void *instr[], void *print[])
{
  int i,r;

  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      if(ea_valid(i, EA_INVALID_A)) {
	instr[0x80c0|(r<<9)|i] = divu;
	print[0x80c0|(r<<9)|i] = divu_print;
      }
    }
  }
}
