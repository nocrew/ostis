#include "common.h"
#include "cpu.h"
#include "mmu.h"

static void subx_b(struct cpu *cpu, WORD op)
{
  BYTE s,d,r;

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
    s = mmu_read_byte(cpu->a[ry]);
    d = mmu_read_byte(cpu->a[rx]);
    r = d-s;
    if(CHKX) r -= 1;
    mmu_write_byte(cpu->a[rx], r);
    ADD_CYCLE(18);
  } else {
    s = cpu->d[ry]&0xff;
    d = cpu->d[rx]&0xff;
    r = d-s;
    if(CHKX) r -= 1;
    cpu->d[rx] = (cpu->d[rx]&0xffffff00)|r;
    ADD_CYCLE(4);
  }

  cpu_set_flags_subx(cpu, s&0x80, d&0x80, r&0x80, r);
}

static void subx_w(struct cpu *cpu, WORD op)
{
  WORD s,d,r;

  int rx,ry;

  rx = (op&0xe00)>>9;
  ry = op&0x7;

  if(op&0x8) {
    cpu->a[ry] -= 2;
    cpu->a[rx] -= 2;
    s = mmu_read_word(cpu->a[ry]);
    d = mmu_read_word(cpu->a[rx]);
    r = d-s;
    if(CHKX) r -= 1;
    mmu_write_word(cpu->a[rx], r);
    ADD_CYCLE(18);
  } else {
    s = cpu->d[ry]&0xffff;
    d = cpu->d[rx]&0xffff;
    r = d-s;
    if(CHKX) r -= 1;
    cpu->d[rx] = (cpu->d[rx]&0xffff0000)|r;
    ADD_CYCLE(4);
  }

  cpu_set_flags_subx(cpu, s&0x8000, d&0x8000, r&0x8000, r);
}

static void subx_l(struct cpu *cpu, WORD op)
{
  LONG s,d,r;

  int rx,ry;

  rx = (op&0xe00)>>9;
  ry = op&0x7;

  if(op&0x8) {
    cpu->a[ry] -= 4;
    cpu->a[rx] -= 4;
    s = mmu_read_long(cpu->a[ry]);
    d = mmu_read_long(cpu->a[rx]);
    r = d-s;
    if(CHKX) r -= 1;
    mmu_write_long(cpu->a[rx], r);
    ADD_CYCLE(30);
  } else {
    s = cpu->d[ry];
    d = cpu->d[rx];
    r = d-s;
    if(CHKX) r -= 1;
    cpu->d[rx] = r;
    ADD_CYCLE(8);
  }

  cpu_set_flags_subx(cpu, s&0x80000000, d&0x80000000, r&0x80000000, r);
}

static void subx(struct cpu *cpu, WORD op)
{
  ENTER;

  switch((op&0xc0)>>6) {
  case 0:
    subx_b(cpu, op);
    return;
  case 1:
    subx_w(cpu, op);
    return;
  case 2:
    subx_l(cpu, op);
    return;
  }
}

static struct cprint *subx_print(LONG addr, WORD op)
{
  int rx,ry,s;
  struct cprint *ret;

  ret = cprint_alloc(addr);
  
  rx = (op&0xe00)>>9;
  ry = op&0x7;
  s = (op&0xc0)>>6;

  switch(s) {
  case 0:
    strcpy(ret->instr, "SUBX.B");
    break;
  case 1:
    strcpy(ret->instr, "SUBX.W");
    break;
  case 2:
    strcpy(ret->instr, "SUBX.L");
    break;
  }
  
  if(op&0x8) {
    sprintf(ret->data, "-(A%d),-(A%d)", ry, rx);
  } else {
    sprintf(ret->data, "D%d,D%d", ry, rx);
  }

  return ret;
}

void subx_init(void *instr[], void *print[])
{
  int rx,ry;

  for(rx=0;rx<8;rx++) {
    for(ry=0;ry<8;ry++) {
      instr[0x9100|(rx<<9)|ry] = subx;
      instr[0x9108|(rx<<9)|ry] = subx;
      instr[0x9140|(rx<<9)|ry] = subx;
      instr[0x9148|(rx<<9)|ry] = subx;
      instr[0x9180|(rx<<9)|ry] = subx;
      instr[0x9188|(rx<<9)|ry] = subx;
      print[0x9100|(rx<<9)|ry] = subx_print;
      print[0x9108|(rx<<9)|ry] = subx_print;
      print[0x9140|(rx<<9)|ry] = subx_print;
      print[0x9148|(rx<<9)|ry] = subx_print;
      print[0x9180|(rx<<9)|ry] = subx_print;
      print[0x9188|(rx<<9)|ry] = subx_print;
    }
  }
}
