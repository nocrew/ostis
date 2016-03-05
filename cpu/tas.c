#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

static void tas(struct cpu *cpu, WORD op)
{
  BYTE s;
  
  s = ea_read_byte(cpu, op&0x3f, 1);
  cpu_set_flags_move(cpu, s&0x80, s);
  ea_set_prefetch_before_write();
  ea_write_byte(cpu, op&0x3f, s|0x80);
  if((op&0x38) == 0) {
    ADD_CYCLE(4);
  } else {
    ADD_CYCLE(14);
  }
}

static struct cprint *tas_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);
  
  strcpy(ret->instr, "TAS");
  ea_print(ret, op&0x3f, 0);

  return ret;
}

void tas_init(void *instr[], void *print[])
{
  int i;

  for(i=0;i<0x40;i++) {
    if(ea_valid(i, EA_INVALID_DST|EA_INVALID_A)) {
      instr[0x4ac0|i] = tas;
      print[0x4ac0|i] = tas_print;
    }
  }
}
