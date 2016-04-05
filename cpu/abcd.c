#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"

static void abcd(struct cpu *cpu, WORD op)
{
  BYTE sb,db,rb;
  int s,d,r;
  int rx,ry;
  
  rx = (op&0xe00)>>9;
  ry = op&0x7;

  if(op&0x8) {
    if(ry == 7)
      cpu->a[ry] -= 2;
    else
      cpu->a[ry] -= 1;
    sb = bus_read_byte(cpu->a[ry]);
    if(rx == 7)
      cpu->a[rx] -= 2;
    else
      cpu->a[rx] -= 1;
    db = bus_read_byte(cpu->a[rx]);
    s = 10*((sb&0xf0)>>4)+((sb&0xf));
    d = 10*((db&0xf0)>>4)+((db&0xf));
    r = s+d;
    if(CHKX) r+=1;
    if(r>=100) { SETC; SETX; } else { CLRC; CLRX; }
    if(r&0xff) SETZ;
    rb = (r%10)|(((r/10)%10)<<4);
    cpu_prefetch();
    bus_write_byte(cpu->a[rx], rb);
    ADD_CYCLE(18);
  } else {
    sb = cpu->d[ry]&0xff;
    db = cpu->d[rx]&0xff;
    s = 10*((sb&0xf0)>>4)+((sb&0xf));
    d = 10*((db&0xf0)>>4)+((db&0xf));
    r = s+d;
    if(CHKX) r+=1;
    if(r>=100) { SETC; SETX; } else { CLRC; CLRX; }
    if(r&0xff) SETZ;
    rb = (r%10)|(((r/10)%10)<<4);
    cpu->d[ry] = (cpu->d[ry]&0xffffff00)|rb;
    ADD_CYCLE(6);
    cpu_prefetch();
  }
}

static struct cprint *abcd_print(LONG addr, WORD op)
{
  int rx,ry;
  struct cprint *ret;

  ret = cprint_alloc(addr);
  
  rx = (op&0xe00)>>9;
  ry = op&0x7;

  strcpy(ret->instr, "ABCD.B");

  if(op&0x8) {
    sprintf(ret->data, "-(A%d),-(A%d)", ry, rx);
  } else {
    sprintf(ret->data, "D%d,D%d", ry, rx);
  }

  return ret;
}

void abcd_init(void *instr[], void *print[])
{
  int rx,ry;

  for(rx=0;rx<8;rx++) {
    for(ry=0;ry<8;ry++) {
      instr[0xc100|(rx<<9)|ry] = abcd;
      instr[0xc108|(rx<<9)|ry] = abcd;
      print[0xc100|(rx<<9)|ry] = abcd_print;
      print[0xc108|(rx<<9)|ry] = abcd_print;
    }
  }
}
