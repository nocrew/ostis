#ifndef EA_H
#define EA_H

#include "common.h"
#include "cpu.h"
#include "mmu.h"

#define MODESWAP(x) (((x&0x38)>>3)|((x&0x7)<<3))
#define ADD_CYCLE_EA(x) do { if(!rmw) { cpu->icycle += x; } } while(0);


#define EA_INVALID_NONE 0x0
#define EA_INVALID_A    0x1
#define EA_INVALID_D    0x2
#define EA_INVALID_I    0x4
#define EA_INVALID_PC   0x8
#define EA_INVALID_INC  0x10
#define EA_INVALID_DEC  0x20

#define EA_INVALID_DST  (EA_INVALID_I|EA_INVALID_PC)
#define EA_INVALID_MEM  (EA_INVALID_DST|EA_INVALID_A|EA_INVALID_D)
#define EA_INVALID_EA   (EA_INVALID_A|EA_INVALID_D|EA_INVALID_INC| \
			 EA_INVALID_DEC|EA_INVALID_I)

#define EA_MODE_REG(mode) (mode&0x07)
#define EA_SIZE_WORD 0
#define EA_SIZE_LONG 1

#define EA_MODE_DREG(mode) ((mode&0x38) == 0x00)
#define EA_MODE_AREG(mode) ((mode&0x38) == 0x08)
#define EA_MODE_AMEM(mode) ((mode&0x38) == 0x10)
#define EA_MODE_AINC(mode) ((mode&0x38) == 0x18)
#define EA_MODE_ADEC(mode) ((mode&0x38) == 0x20)
#define EA_MODE_AOFF(mode) ((mode&0x38) == 0x28)
#define EA_MODE_AROF(mode) ((mode&0x38) == 0x30)
#define EA_MODE_SHRT(mode) ((mode&0x3f) == 0x38)
#define EA_MODE_LONG(mode) ((mode&0x3f) == 0x39)
#define EA_MODE_POFF(mode) ((mode&0x3f) == 0x3a)
#define EA_MODE_PROF(mode) ((mode&0x3f) == 0x3b)
#define EA_MODE_IMDT(mode) ((mode&0x3f) == 0x3c)

#define EA_MODE_FETCH_0(mode) (EA_MODE_AMEM(mode) || EA_MODE_AINC(mode) || EA_MODE_ADEC(mode))
#define EA_MODE_FETCH_1(mode) (EA_MODE_AOFF(mode) || EA_MODE_AROF(mode) || EA_MODE_SHRT(mode) || EA_MODE_POFF(mode) || EA_MODE_PROF(mode))
#define EA_MODE_FETCH_2(mode) (EA_MODE_LONG(mode))

#define EA_FETCH_COUNT(mode) (EA_MODE_FETCH_0(mode) ? 0 : EA_MODE_FETCH_1(mode) ? 1 : EA_MODE_FETCH_2(mode) ? 2 : -1)

static LONG ea_addr(struct cpu *cpu, int size, WORD mode, WORD *words)
{
  LONG addr = 0;
  LONG disp;
  
  if(EA_MODE_DREG(mode) || EA_MODE_AREG(mode)) {
    return 0;
  }

  if(EA_MODE_AMEM(mode) || EA_MODE_AOFF(mode) ||
     EA_MODE_AINC(mode) || EA_MODE_ADEC(mode) || EA_MODE_AROF(mode)) {
    addr = cpu->a[EA_MODE_REG(mode)];
  }

  if(EA_MODE_AOFF(mode)) {
    addr += EXTEND_WORD(words[0]);
  }

  if(EA_MODE_POFF(mode)) {
    addr = cpu->pc;
    addr += EXTEND_WORD(words[0]);
  }

  if(EA_MODE_AROF(mode)) {
    addr += EXTEND_BY2L(words[0]&0xff);
    if(words[0]&0x8000) {
      disp = cpu->a[(words[0]&0x7000)>>12];
    } else {
      disp = cpu->d[(words[0]&0x7000)>>12];
    }
    if(words[0]&0x800) {
      disp = EXTEND_WORD(disp&0xffff);
    }
    addr += disp;
  }

  if(EA_MODE_PROF(mode)) {
    addr = cpu->pc;
    addr += EXTEND_BY2L(words[0]&0xff);
    if(words[0]&0x8000) {
      disp = cpu->a[(words[0]&0x7000)>>12];
    } else {
      disp = cpu->d[(words[0]&0x7000)>>12];
    }
    if(words[0]&0x800) {
      disp = EXTEND_WORD(disp&0xffff);
    }
    addr += disp;
  }

  if(EA_MODE_SHRT(mode)) {
    addr = EXTEND_WORD(words[0]);
  }

  if(EA_MODE_LONG(mode)) {
    addr = (words[0]<<16)|words[1];
  }

  return addr;
}

BYTE ea_read_byte(struct cpu *,int,int);
WORD ea_read_word(struct cpu *,int,int);
LONG ea_read_long(struct cpu *,int,int);
void ea_write_byte(struct cpu *,int,BYTE);
void ea_write_word(struct cpu *,int,WORD);
void ea_write_long(struct cpu *,int,LONG);
LONG ea_get_addr(struct cpu *,int);
void ea_print(struct cprint *,int, int);
int ea_valid(int, int);
void ea_set_prefetch_before_write();
void ea_clear_prefetch_before_write();

void ea_begin_read(struct cpu *cpu, WORD op);
void ea_begin_write(struct cpu *cpu, WORD op);
void ea_begin_modify(struct cpu *cpu, WORD op, LONG data, int, int, int, int);
int ea_step(LONG *);

#endif
