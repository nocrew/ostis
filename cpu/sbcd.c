#include "common.h"
#include "cpu.h"
#include "mmu.h"

static void sbcd(struct cpu *cpu, WORD op)
{
  BYTE sb,db,rb;
  int s,d,r;
  int rx,ry;
  
  rx = (op&0xe00)>>9;
  ry = op&0x7;

  if(op&0x8) {
    if(rx == 7)
      cpu->a[rx] -= 2;
    else
      cpu->a[rx] -= 1;
    if(ry == 7)
      cpu->a[ry] -= 2;
    else
      cpu->a[ry] -= 1;
    sb = mmu_read_byte(cpu->a[ry]);
    db = mmu_read_byte(cpu->a[rx]);
    s = 10*((sb&0xf0)>>4)+((sb&0xf));
    d = 10*((db&0xf0)>>4)+((db&0xf));
    r = d-s;
    if(CHKX) r-=1;
    if(r<0) { SETC; SETX; } else { CLRC; CLRX; }
    if(r&0xff) SETZ;
    rb = (r%10)|(((r/10)%10)<<4);
    mmu_write_byte(cpu->a[rx], rb);
    ADD_CYCLE(18);
  } else {
    sb = cpu->d[ry]&0xff;
    db = cpu->d[rx]&0xff;
    s = 10*((sb&0xf0)>>4)+((sb&0xf));
    d = 10*((db&0xf0)>>4)+((db&0xf));
    r = d-s;
    if(CHKX) r-=1;
    if(r<0) { SETC; SETX; } else { CLRC; CLRX; }
    if(r&0xff) SETZ;
    rb = (r%10)|(((r/10)%10)<<4);
    cpu->d[ry] = (cpu->d[ry]&0xffffff00)|rb;
    ADD_CYCLE(6);
  }
}

static struct cprint *sbcd_print(LONG addr, WORD op)
{
  int rx,ry;
  struct cprint *ret;

  ret = cprint_alloc(addr);
  
  rx = (op&0xe00)>>9;
  ry = op&0x7;

  strcpy(ret->instr, "SBCD.B");

  if(op&0x8) {
    sprintf(ret->data, "-(A%d),-(A%d)", ry, rx);
  } else {
    sprintf(ret->data, "D%d,D%d", ry, rx);
  }

  return ret;
}

void sbcd_init(void *instr[], void *print[])
{
  int rx,ry;

  for(rx=0;rx<8;rx++) {
    for(ry=0;ry<8;ry++) {
      instr[0x8100|(rx<<9)|ry] = sbcd;
      instr[0x8108|(rx<<9)|ry] = sbcd;
      print[0x8100|(rx<<9)|ry] = sbcd_print;
      print[0x8108|(rx<<9)|ry] = sbcd_print;
    }
  }
}
