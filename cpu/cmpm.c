#include "common.h"
#include "cpu.h"
#include "mmu.h"

static void cmpm_b(struct cpu *cpu, WORD op)
{
  int rx,ry;
  BYTE s,d,r;
  
  rx = (op&0xe00)>>9;
  ry = op&0x7;
  
  s = mmu_read_byte(cpu->a[ry]);
  if(ry == 7)
    cpu->a[ry] += 2;
  else
    cpu->a[ry] += 1;
  d = mmu_read_byte(cpu->a[rx]);
  if(rx == 7)
    cpu->a[rx] += 2;
  else
    cpu->a[rx] += 1;
  r = d-s;
  
  ADD_CYCLE(12);
  cpu_set_flags_cmp(cpu, s&0x80, d&0x80, r&0x80, r);
}

static void cmpm_w(struct cpu *cpu, WORD op)
{
  int rx,ry;
  WORD s,d,r;

  rx = (op&0xe00)>>9;
  ry = op&0x7;
  
  s = mmu_read_word(cpu->a[ry]);
  cpu->a[ry] += 2;
  d = mmu_read_word(cpu->a[rx]);
  cpu->a[rx] += 2;
  r = d-s;
  
  ADD_CYCLE(12);
  cpu_set_flags_cmp(cpu, s&0x8000, d&0x8000, r&0x8000, r);
}

static void cmpm_l(struct cpu *cpu, WORD op)
{
  int rx,ry;
  LONG s,d,r;
  
  rx = (op&0xe00)>>9;
  ry = op&0x7;
  
  s = mmu_read_long(cpu->a[ry]);
  cpu->a[ry] += 4;
  d = mmu_read_long(cpu->a[rx]);
  cpu->a[rx] += 4;
  r = d-s;
  
  ADD_CYCLE(20);
  cpu_set_flags_cmp(cpu, s&0x80000000, d&0x80000000, r&0x80000000, r);
}

static void cmpm(struct cpu *cpu, WORD op)
{
  ENTER;
  
  switch((op&0xc0)>>6) {
  case 0:
    cmpm_b(cpu, op);
    return;
  case 1:
    cmpm_w(cpu, op);
    return;
  case 2:
    cmpm_l(cpu, op);
    return;
  }
}

static struct cprint *cmpm_print(LONG addr, WORD op)
{
  int rx,ry,s;
  struct cprint *ret;
  
  ret = cprint_alloc(addr);

  rx = (op&0xe00)>>9;
  ry = op&0x7;
  s = (op&0xc0)>>6;
  
  switch(s) {
  case 0:
    strcpy(ret->instr, "CMPM.B");
    break;
  case 1:
    strcpy(ret->instr, "CMPM.W");
    break;
  case 2:
    strcpy(ret->instr, "CMPM.L");
    break;
  }
  sprintf(ret->data, "(A%d)+,(A%d)+", ry, rx);
  
  return ret;
}

void cmpm_init(void *instr[], void *print[])
{
  int rx,ry;

  for(rx=0;rx<8;rx++) {
    for(ry=0;ry<8;ry++) {
      instr[0xb108|(rx<<9)|ry] = cmpm;
      instr[0xb148|(rx<<9)|ry] = cmpm;
      instr[0xb188|(rx<<9)|ry] = cmpm;
      print[0xb108|(rx<<9)|ry] = cmpm_print;
      print[0xb148|(rx<<9)|ry] = cmpm_print;
      print[0xb188|(rx<<9)|ry] = cmpm_print;
    }
  }

}
