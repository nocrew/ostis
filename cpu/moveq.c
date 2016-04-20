#include "common.h"
#include "cpu.h"
#include "cprint.h"

static void moveq(struct cpu *cpu, WORD op)
{
  LONG s;
  int r;
  
  ENTER;

  ADD_CYCLE(4);
  r = (op&0xe00)>>9;
  s = op&0xff;
  if(s&0x80) s |= 0xffffff00;
  cpu->d[r] = s;
  
  cpu_set_flags_move(cpu, s&0x80, s);
  cpu_prefetch();
}

static struct cprint *moveq_print(LONG addr, WORD op)
{
  int r;
  BYTE s;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  s = op&0xff;
  r = (op&0xe00)>>9;

  strcpy(ret->instr, "MOVEQ");
  sprintf(ret->data, "#%d,D%d", s, r);

  return ret;
}

void moveq_init(void *instr[], void *print[])
{
  int i,r;

  for(r=0;r<8;r++) {
    for(i=0;i<0x100;i++) {
      instr[0x7000|(r<<9)|i] = moveq;
      print[0x7000|(r<<9)|i] = moveq_print;
    }
  }
}
