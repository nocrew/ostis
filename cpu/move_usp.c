#include "common.h"
#include "cpu.h"
#include "cprint.h"

static void move_usp(struct cpu *cpu, WORD op)
{
  ENTER;

  if(cpu->sr&0x2000) {
    if(op&0x8) {
      cpu->a[op&7] = cpu->usp;
    } else {
      cpu->usp = cpu->a[op&7];
    }
    ADD_CYCLE(4);
    cpu_prefetch();
  } else {
    cpu_set_exception(8);
  }
}

static struct cprint *move_usp_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, "MOVE");
  if(op&0x8) {
    sprintf(ret->data, "USP,A%d", op&0x7);
  } else {
    sprintf(ret->data, "A%d,USP", op&0x7);
  }
  
  return ret;
}

void move_usp_init(void *instr[], void *print[])
{
  int i;
  
  for(i=0;i<0x10;i++) {
    instr[0x4e60|i] = move_usp;
    print[0x4e60|i] = move_usp_print;
  }
}
