#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"
#include "ea.h"

static void cmp_b(struct cpu *cpu, int reg, int mode)
{
  BYTE s,d,r;

  ADD_CYCLE(4);
  s = ea_read_byte(cpu, mode, 0);
  d = cpu->d[reg]&0xff;
  r = d-s;
  cpu_set_flags_cmp(cpu, s&0x80, d&0x80, r&0x80, r);
}

static void cmp_w(struct cpu *cpu, int reg, int mode)
{
  WORD s,d,r;

  ADD_CYCLE(4);
  s = ea_read_word(cpu, mode, 0);
  d = cpu->d[reg]&0xffff;
  r = d-s;
  cpu_set_flags_cmp(cpu, s&0x8000, d&0x8000, r&0x8000, r);
}

static void cmp_l(struct cpu *cpu, int reg, int mode)
{
  LONG s,d,r;

  ADD_CYCLE(6);
  s = ea_read_long(cpu, mode, 0);
  d = cpu->d[reg];
  r = d-s;
  cpu_set_flags_cmp(cpu, s&0x80000000, d&0x80000000, r&0x80000000, r);
}

static void cmp(struct cpu *cpu, WORD op)
{
  int r;
  
  ENTER;

  r = (op&0xe00)>>9;
  switch((op&0xc0)>>6) {
  case 0:
    cmp_b(cpu, r, op&0x3f);
    break;
  case 1:
    cmp_w(cpu, r, op&0x3f);
    break;
  case 2:
    cmp_l(cpu, r, op&0x3f);
    break;
  }
  cpu_prefetch();
}

static struct cprint *cmp_print(LONG addr, WORD op)
{
  int r;
  int s;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  r = (op&0xe00)>>9;
  s = (op&0xc0)>>6;

  switch(s) {
  case 0:
    strcpy(ret->instr, "CMP.B");
    break; 
  case 1:
    strcpy(ret->instr, "CMP.W");
    break;
  case 2:
    strcpy(ret->instr, "CMP.L");
    break;
  }
  ea_print(ret, op&0x3f, s);
  sprintf(ret->data, "%s,D%d", ret->data, r);

  return ret;
}

void cmp_init(void *instr[], void *print[])
{
  int r,i;
  
  for(r=0;r<8;r++) {
    for(i=0;i<0x40;i++) {
      if(ea_valid(i, EA_INVALID_A)) {
	instr[0xb000|(r<<9)|i] = cmp;
	print[0xb000|(r<<9)|i] = cmp_print;
      }
      if(ea_valid(i, EA_INVALID_NONE)) {
	instr[0xb040|(r<<9)|i] = cmp;
	instr[0xb080|(r<<9)|i] = cmp;
	print[0xb040|(r<<9)|i] = cmp_print;
	print[0xb080|(r<<9)|i] = cmp_print;
      }
    }
  }
}



