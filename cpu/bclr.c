#include "common.h"
#include "cpu.h"
#include "mmu.h"
#include "ea.h"

static void bclr_i(struct cpu *cpu, int mode)
{
  BYTE b,d;

  b = (BYTE)mmu_read_word(cpu->pc)&0xff;
  b &= 31;
  cpu->pc += 2;
  if(mode&0x38) {
    ADD_CYCLE(8);
    d = ea_read_byte(cpu, mode, 1);
    if(d & (1<<(b&7))) CLRZ; else SETZ;
    d &= ~(1<<(b&7));
    ea_write_byte(cpu, mode, d);
  } else {
    ADD_CYCLE(10);
    if(cpu->d[mode&7] & (1<<b)) CLRZ; else SETZ;
    cpu->d[mode&7] &= ~(1<<b);
  }
}

static void bclr_r(struct cpu *cpu, int reg, int mode)
{
  BYTE b,d;
  
  b = cpu->d[reg]&31;
  if(mode&0x38) {
    ADD_CYCLE(4);
    d = ea_read_byte(cpu, mode, 1);
    if(d & (1<<(b&7))) CLRZ; else SETZ;
    d &= ~(1<<(b&7));
    ea_write_byte(cpu, mode, d);
  } else {
    ADD_CYCLE(6);
    if(cpu->d[mode&7] & (1<<b)) CLRZ; else SETZ;
    cpu->d[mode&7] &= ~(1<<b);
  }
}

static void bclr(struct cpu *cpu, WORD op)
{
  ENTER;

  switch(op&0x100) {
  case 0:
    bclr_i(cpu, op&0x3f);
    break;
  case 0x100:
    bclr_r(cpu, (op&0xe00)>>9, op&0x3f);
    break;
  }
}

static struct cprint *bclr_print(LONG addr, WORD op)
{
  int b;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  switch((op&0x100)>>8) {
  case 0:
    b = mmu_read_word_print(addr+ret->size)&0x1f;
    ret->size += 2;
    if(op&0x38) {
      b &= 7;
      strcpy(ret->instr, "BCLR.B");
    } else {
      strcpy(ret->instr, "BCLR.L");
    }
    sprintf(ret->data, "#%d,", b);
    ea_print(ret, op&0x3f, 0);
    break;
  case 1:
    b = (op&0xe00)>>9;
    if(op&0x38) {
      strcpy(ret->instr, "BCLR.B");
    } else {
      strcpy(ret->instr, "BCLR.L");
    }
    sprintf(ret->data, "D%d,", b);
    ea_print(ret, op&0x3f, 0);
    break;
  }

  return ret;
}

void bclr_init(void *instr[], void *print[])
{
  int i,r;
  for(i=0;i<0x40;i++) {
    for(r=0;r<8;r++) {
      instr[0x0180|(r<<9)|i] = bclr;
      print[0x0180|(r<<9)|i] = bclr_print;
    }
    instr[0x0880|i] = bclr;
    print[0x0880|i] = bclr_print;
  }
}
