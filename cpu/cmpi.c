#include "common.h"
#include "cpu.h"
#include "mmu.h"
#include "ea.h"

static void cmpi_b(struct cpu *cpu, int mode)
{
  BYTE i,e,r;

  ADD_CYCLE(8);
  i = mmu_read_word(cpu->pc)&0xff;
  cpu->pc += 2;
  e = ea_read_byte(cpu, mode, 0);

  r = e-i;
  cpu_set_flags_sub(cpu, i&0x80, e&0x80, r&0x80, r);
}

static void cmpi_w(struct cpu *cpu, int mode)
{
  WORD i,e,r;

  ADD_CYCLE(8);
  i = mmu_read_word(cpu->pc);
  cpu->pc += 2;
  e = ea_read_word(cpu, mode, 0);

  r = e-i;
  cpu_set_flags_sub(cpu, i&0x8000, e&0x8000, r&0x8000, r);
}

static void cmpi_l(struct cpu *cpu, int mode)
{
  LONG i,e,r;

  if(!(mode&0x38)) {
      ADD_CYCLE(14);
  }
  i = mmu_read_long(cpu->pc);
  cpu->pc += 4;
  e = ea_read_long(cpu, mode, 0);
  
  r = e-i;
  cpu_set_flags_sub(cpu, i&0x80000000, e&0x80000000, r&0x80000000, r);
}

static void cmpi(struct cpu *cpu, WORD op)
{
  ENTER;

  switch((op&0xc0)>>6) {
  case 0:
    cmpi_b(cpu, op&0x3f);
    return;
  case 1:
    cmpi_w(cpu, op&0x3f);
    return;
  case 2:
    cmpi_l(cpu, op&0x3f);
    return;
  }
}

static struct cprint *cmpi_print(LONG addr, WORD op)
{
  int s;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  s = (op&0xc0)>>6;

  switch(s) {
  case 0:
    strcpy(ret->instr, "CMPI.B");
    sprintf(ret->data, "#$%x,", mmu_read_word_print(addr+ret->size)&0xff);
    ret->size += 2;
    break;
  case 1:
    strcpy(ret->instr, "CMPI.W");
    sprintf(ret->data, "#$%x,", mmu_read_word_print(addr+ret->size));
    ret->size += 2;
    break;
  case 2:
    strcpy(ret->instr, "CMPI.L");
    sprintf(ret->data, "#$%x,", mmu_read_long_print(addr+ret->size));
    ret->size += 4;
    break;
  }
  ea_print(ret, op&0x3f, s);

  return ret;
}

void cmpi_init(void *instr[], void *print[])
{
  int i;
  for(i=0;i<0xc0;i++) {
    instr[0x0c00|i] = (void *)cmpi;
    print[0x0c00|i] = (void *)cmpi_print;
  }
}







