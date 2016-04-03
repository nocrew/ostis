#include "common.h"
#include "cpu.h"
#include "cprint.h"

#define EXG_IN_PROGRESS 1

static void exg(struct cpu *cpu, WORD op)
{
  static uint32_t start;
  LONG t;
  int rx,ry;

  ENTER;

  if(cpu->instr_state == INSTR_STATE_NONE) {
    start = cpu->cycle;
    cpu->instr_state = EXG_IN_PROGRESS;
  }

  switch(cpu->cycle - start) {
  case 0:
    cpu_prefetch();
    break;
  case 4:
    rx = (op&0xe00)>>9;
    ry = (op&7);
    switch(op&0xf8) {
    case 0x40: /* Dx,Dy */
      t = cpu->d[rx];
      cpu->d[rx] = cpu->d[ry];
      cpu->d[ry] = t;
      break;
    case 0x48:/* Ax,Ay */
      t = cpu->a[rx];
      cpu->a[rx] = cpu->a[ry];
      cpu->a[ry] = t;
      break;
    case 0x88:/* Dx,Ay */
      t = cpu->d[rx];
      cpu->d[rx] = cpu->a[ry];
      cpu->a[ry] = t;
      break;
    }
    ADD_CYCLE(2);
    cpu->instr_state = INSTR_STATE_FINISHED;
    break;
  }
}

static struct cprint *exg_print(LONG addr, WORD op)
{
  int rx,ry;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  rx = (op&0xe00)>>9;
  ry = op&0x7;

  strcpy(ret->instr, "EXG");

  if(((op&0xf8)>>3) == 8) {
    sprintf(ret->data, "D%d,D%d", rx, ry);
  } else if(((op&0xf8)>>3) == 9) {
    sprintf(ret->data, "A%d,A%d", rx, ry);
  } else if(((op&0xf8)>>3) == 17) {
    sprintf(ret->data, "D%d,A%d", rx, ry);
  }

  return ret;
}

void exg_instr_init(void *instr[], void *print[])
{
  int rx,ry;

  for(rx=0;rx<8;rx++) {
    for(ry=0;ry<8;ry++) {
      instr[0xc140|(rx<<9)|ry] = exg;
      instr[0xc148|(rx<<9)|ry] = exg;
      instr[0xc188|(rx<<9)|ry] = exg;
      print[0xc140|(rx<<9)|ry] = exg_print;
      print[0xc148|(rx<<9)|ry] = exg_print;
      print[0xc188|(rx<<9)|ry] = exg_print;
    }
  }
}
