#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

static void muls(struct cpu *cpu, WORD op)
{
  int s,d,r;
  int reg;
  int i,c;

  ENTER;

  s = ea_read_word(cpu, op&0x3f, 0);
  if(s&0x8000) s |= 0xffff0000;
  reg = (op&0xe00)>>9;
  d = cpu->d[reg]&0xffff;
  if(d&0x8000) d |= 0xffff0000;
  r = s*d;
  cpu->d[reg] = r;
  ADD_CYCLE(38);
  c = s<<1;
  for(i=16;i>=1;i--) {
    if(((c&(1<<i)) &&
	!(c&(1<<(i-1))))) {
      ADD_CYCLE(2);
    }
  }
  cpu_set_flags_move(cpu, r&0x80000000, r);
}

static struct cprint *muls_print(LONG addr, WORD op)
{
  int r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  r = (op&0xe00)>>9;

  strcpy(ret->instr, "MULS");
  ea_print(ret, op&0x3f, 1);
  sprintf(ret->data, "%s,D%d", ret->data, r);

  return ret;
}

void muls_init(void *instr[], void *print[])
{
  int i,r;

  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      if(ea_valid(i, EA_INVALID_A)) {
	instr[0xc1c0|(r<<9)|i] = muls;
	print[0xc1c0|(r<<9)|i] = muls_print;
      }
    }
  }
}
