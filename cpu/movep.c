#include "common.h"
#include "cpu.h"
#include "mmu.h"

static void movep(struct cpu *cpu, WORD op)
{
  LONG d,a;
  int dr,ar;

  ENTER;

  ar = op&0x7;
  dr = (op&0xe00)>>9;
  a = mmu_read_word(cpu->pc)+cpu->a[ar];
  cpu->pc += 2;
  
  switch((op&0xc0)>>6) {
  case 0:
    d = (mmu_read_byte(a)<<8)|mmu_read_byte(a+2);
    cpu->d[dr] = (cpu->d[dr]&0xffff0000)|(d&0xffff);
    ADD_CYCLE(16);
    return;
  case 1:
    d = ((mmu_read_byte(a)<<24)|
	 (mmu_read_byte(a+2)<<16)|
	 (mmu_read_byte(a+4)<<8)|
	 (mmu_read_byte(a+6)));
    cpu->d[dr] = d;
    ADD_CYCLE(24);
    return;
  case 2:
    d = cpu->d[dr]&0xffff;
    mmu_write_byte(a, (BYTE)((d&0xff00)>>8));
    mmu_write_byte(a+2, (BYTE)((d&0xff)));
    ADD_CYCLE(16);
    return;
  case 3:
    d = cpu->d[dr];
    mmu_write_byte(a, (BYTE)((d&0xff000000)>>24));
    mmu_write_byte(a+2, (BYTE)((d&0xff0000)>>16));
    mmu_write_byte(a+4, (BYTE)((d&0xff00)>>8));
    mmu_write_byte(a+6, (BYTE)((d&0xff)));
    ADD_CYCLE(24);
    return;
  }
}

static struct cprint *movep_print(LONG addr, WORD op)
{
  int ar,dr,o;
  struct cprint *ret;
  
  ret = cprint_alloc(addr);
  
  ar = op&0x7;
  dr = (op&0xe00)>>9;
  o = mmu_read_word(addr+ret->size);
  ret->size += 2;

  switch((op&0xc0)>>6) {
  case 0:
    strcpy(ret->instr, "MOVEP.W");
    sprintf(ret->data, "%d(A%d),D%d", o, ar, dr);
    break;
  case 1:
    strcpy(ret->instr, "MOVEP.L");
    sprintf(ret->data, "%d(A%d),D%d", o, ar, dr);
    break;
  case 2:
    strcpy(ret->instr, "MOVEP.W");
    sprintf(ret->data, "D%d,%d(A%d)", dr, o, ar);
    break;
  case 3:
    strcpy(ret->instr, "MOVEP.L");
    sprintf(ret->data, "D%d,%d(A%d)", dr, o, ar);
    break;
  }

  return ret;
}

void movep_init(void *instr[], void *print[])
{
  int d,a;

  for(d=0;d<8;d++) {
    for(a=0;a<8;a++) {
      instr[0x0108|(d<<9)|a] = movep;
      instr[0x0148|(d<<9)|a] = movep;
      instr[0x0188|(d<<9)|a] = movep;
      instr[0x01c8|(d<<9)|a] = movep;
      print[0x0108|(d<<9)|a] = movep_print;
      print[0x0148|(d<<9)|a] = movep_print;
      print[0x0188|(d<<9)|a] = movep_print;
      print[0x01c8|(d<<9)|a] = movep_print;
    }
  }
}
