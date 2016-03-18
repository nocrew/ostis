#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"

static void addx_b(struct cpu *cpu, WORD op)
{
  BYTE s,d,r;

  int rx,ry;

  rx = (op&0xe00)>>9;
  ry = op&0x7;

  if(op&0x8) {
    if(ry == 7)
      cpu->a[ry] -= 2;
    else
      cpu->a[ry] -= 1;
    s = bus_read_byte(cpu->a[ry]);
    if(rx == 7)
      cpu->a[rx] -= 2;
    else
      cpu->a[rx] -= 1;
    d = bus_read_byte(cpu->a[rx]);
    r = s+d;
    if(CHKX) r += 1;
    cpu_prefetch();
    bus_write_byte(cpu->a[rx], r);
    ADD_CYCLE(18);
  } else {
    s = cpu->d[ry]&0xff;
    d = cpu->d[rx]&0xff;
    r = s+d;
    if(CHKX) r += 1;
    cpu->d[rx] = (cpu->d[rx]&0xffffff00)|r;
    ADD_CYCLE(4);
  }

  cpu_set_flags_addx(cpu, s&0x80, d&0x80, r&0x80, r);
}

static void addx_w(struct cpu *cpu, WORD op)
{
  WORD s,d,r;

  int rx,ry;

  rx = (op&0xe00)>>9;
  ry = op&0x7;

  if(op&0x8) {
    cpu->a[ry] -= 2;
    s = bus_read_word(cpu->a[ry]);
    cpu->a[rx] -= 2;
    d = bus_read_word(cpu->a[rx]);
    r = s+d;
    if(CHKX) r += 1;
    cpu_prefetch();
    bus_write_word(cpu->a[rx], r);
    ADD_CYCLE(18);
  } else {
    s = cpu->d[ry]&0xffff;
    d = cpu->d[rx]&0xffff;
    r = s+d;
    if(CHKX) r += 1;
    cpu->d[rx] = (cpu->d[rx]&0xffff0000)|r;
    ADD_CYCLE(4);
  }

  cpu_set_flags_addx(cpu, s&0x8000, d&0x8000, r&0x8000, r);
}

static void addx_l(struct cpu *cpu, WORD op)
{
  LONG s,d,r;

  int rx,ry;

  rx = (op&0xe00)>>9;
  ry = op&0x7;

  if(op&0x8) {
    cpu->a[ry] -= 4;
    s = bus_read_long(cpu->a[ry]);
    cpu->a[rx] -= 4;
    d = bus_read_long(cpu->a[rx]);
    r = s+d;
    if(CHKX) r += 1;
    cpu_prefetch();
    bus_write_long(cpu->a[rx], r);
    ADD_CYCLE(30);
  } else {
    s = cpu->d[ry];
    d = cpu->d[rx];
    r = s+d;
    if(CHKX) r += 1;
    cpu->d[rx] = r;
    ADD_CYCLE(8);
  }

  cpu_set_flags_addx(cpu, s&0x80000000, d&0x80000000, r&0x80000000, r);
}

static void addx(struct cpu *cpu, WORD op)
{
  ENTER;

  switch((op&0xc0)>>6) {
  case 0:
    addx_b(cpu, op);
    return;
  case 1:
    addx_w(cpu, op);
    return;
  case 2:
    addx_l(cpu, op);
    return;
  }
}

static struct cprint *addx_print(LONG addr, WORD op)
{
  int rx,ry,s;
  struct cprint *ret;

  ret = cprint_alloc(addr);
  
  rx = (op&0xe00)>>9;
  ry = op&0x7;
  s = (op&0xc0)>>6;

  switch(s) {
  case 0:
    strcpy(ret->instr, "ADDX.B");
    break;
  case 1:
    strcpy(ret->instr, "ADDX.W");
    break;
  case 2:
    strcpy(ret->instr, "ADDX.L");
    break;
  }
  
  if(op&0x8) {
    sprintf(ret->data, "-(A%d),-(A%d)", ry, rx);
  } else {
    sprintf(ret->data, "D%d,D%d", ry, rx);
  }

  return ret;
}

void addx_init(void *instr[], void *print[])
{
  int rx,ry;

  for(rx=0;rx<8;rx++) {
    for(ry=0;ry<8;ry++) {
      instr[0xd100|(rx<<9)|ry] = addx;
      instr[0xd108|(rx<<9)|ry] = addx;
      instr[0xd140|(rx<<9)|ry] = addx;
      instr[0xd148|(rx<<9)|ry] = addx;
      instr[0xd180|(rx<<9)|ry] = addx;
      instr[0xd188|(rx<<9)|ry] = addx;
      print[0xd100|(rx<<9)|ry] = addx_print;
      print[0xd108|(rx<<9)|ry] = addx_print;
      print[0xd140|(rx<<9)|ry] = addx_print;
      print[0xd148|(rx<<9)|ry] = addx_print;
      print[0xd180|(rx<<9)|ry] = addx_print;
      print[0xd188|(rx<<9)|ry] = addx_print;
    }
  }
}
