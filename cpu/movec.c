#include "common.h"
#include "cpu.h"
#include "ea.h"

static void movec(struct cpu *cpu, WORD op)
{
  ENTER;

  cpu->pc += 2;
  cpu_set_exception(4); /* illegal instruction */
}

static void movec_print(struct cpu *cpu, WORD op)
{
  printf("ILLEGAL (MOVEC)\n");
}

void movec_init(void *instr[])
{
  instr[0x4e7a] = (void *)movec;
  instr[0x4e7b] = (void *)movec;
  instr[0x14e7a] = (void *)movec;
  instr[0x14e7b] = (void *)movec;
}
