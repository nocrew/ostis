#include "common.h"
#include "cpu.h"
#include "ea.h"

/* This is really a 68010+ instruction. Not sure why this was implemented :) */

static void move_from_ccr(struct cpu *cpu, WORD op)
{
  ENTER;

  ADD_CYCLE(8); /* really 6 for EA==reg, but that will be 8 anyway */
  ea_write_word(cpu, op&0x3f, cpu->sr&0xff);
}

static void move_from_ccr_print(struct cpu *cpu, WORD op)
{
  printf("MOVE\tCCR,");
  ea_print(cpu, op&0x3f, 0, 1);
  printf("\n");
}

void move_from_ccr_init(void *instr[])
{
  int i;
  for(i=0;i<0x40;i++) {
    if(ea_valid(i, EA_INVALID_DST|EA_INVALID_A)) {
      instr[0x42c0|i] = (void *)move_from_ccr;
      instr[0x142c0|i] = (void *)move_from_ccr_print;
    }
  }
}
