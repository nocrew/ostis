#include "common.h"
#include "cpu.h"
#include "ea.h"

static void mulu(struct cpu *cpu, WORD op)
{
  LONG s,d,r;
  int reg;
  int i;

  ENTER;

  s = ea_read_word(cpu, op&0x3f, 0);
  reg = (op&0xe00)>>9;
  d = cpu->d[reg]&0xffff;
  r = s*d;
  cpu->d[reg] = r;
  ADD_CYCLE(38);
  for(i=0;i<16;i++) {
    if(s&(1<<i)) {
      ADD_CYCLE(2);
    }
  }
  cpu_set_flags_move(cpu, r&0x80000000, r);
}

static struct cprint *mulu_print(LONG addr, WORD op)
{
  int r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  r = (op&0xe00)>>9;

  strcpy(ret->instr, "MULU");
  ea_print(ret, op&0x3f, 1);
  sprintf(ret->data, "%s,D%d", ret->data, r);

  return ret;
}

void mulu_init(void *instr[], void *print[])
{
  int i,r;

  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      instr[0xc0c0|(r<<9)|i] = mulu;
      print[0xc0c0|(r<<9)|i] = mulu_print;
    }
  }
}
