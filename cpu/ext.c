#include "common.h"
#include "cpu.h"

static void ext(struct cpu *cpu, WORD op)
{
  LONG d;
  int r;

  r = op&0x7;
  
  d = cpu->d[r];
  if(op&0x40) {
    d &= 0xffff;
    if(d&0x8000) d |= 0xffff0000;
    cpu->d[r] = d;
    cpu_set_flags_move(cpu, d&0x80000000, d);
  } else {
    d &= 0xff;
    if(d&0x80) d |= 0xff00;
    cpu->d[r] = (cpu->d[r]&0xffff0000)|(d&0xffff);
    cpu_set_flags_move(cpu, d&0x8000, d);
  }
  ADD_CYCLE(4);
}

static struct cprint *ext_print(LONG addr, WORD op)
{
  int r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  r = op&0x7;

  sprintf(ret->instr, "EXT.%c", (op&0x40)?'L':'W');
  sprintf(ret->data, "D%d", r);
  
  return ret;
}

void ext_init(void *instr[], void *print[])
{
  int r;

  for(r=0;r<8;r++) {
    instr[0x4880|r] = ext;
    instr[0x48c0|r] = ext;
    print[0x4880|r] = ext_print;
    print[0x48c0|r] = ext_print;
  }
}
