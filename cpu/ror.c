#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

static void ror_r(struct cpu *cpu, WORD op)
{
  int c,r;
  BYTE b;
  WORD w;
  LONG l;
  
  c = cpu->d[(op&0xe00)>>9]&63;
  r = (op&7);

  ADD_CYCLE(6+c*2);

  switch((op&0xc0)>>6) {
  case 0:
    b = ((cpu->d[r]&0xff)>>c)|((cpu->d[r]&0xff)<<(8-c));
    cpu_set_flags_rol(cpu, b&0x80, b, c, ((cpu->d[r]&0xff)>>(c-1))&0x1);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    w = ((cpu->d[r]&0xffff)>>c)|((cpu->d[r]&0xffff)<<(16-c));
    cpu_set_flags_rol(cpu, w&0x8000, w, c, ((cpu->d[r]&0xffff)>>(c-1))&0x1);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    l = (cpu->d[r]>>c)|(cpu->d[r]<<(32-c));
    cpu_set_flags_rol(cpu, l&0x80000000, l, c, (cpu->d[r]>>(c-1))&0x1);
    cpu->d[r] = l;
    break;
  }
}

static void ror_i(struct cpu *cpu, WORD op)
{
  int c,r;
  BYTE b;
  WORD w;
  LONG l;

  c = (op&0xe00)>>9;
  if(c == 0) c = 8;
  r = (op&7);
  
  ADD_CYCLE(6+c*2);

  switch((op&0xc0)>>6) {
  case 0:
    b = ((cpu->d[r]&0xff)>>c)|((cpu->d[r]&0xff)<<(8-c));
    cpu_set_flags_rol(cpu, b&0x80, b, c, ((cpu->d[r]&0xff)>>(c-1))&0x1);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    w = ((cpu->d[r]&0xffff)>>c)|((cpu->d[r]&0xffff)<<(16-c));
    cpu_set_flags_rol(cpu, w&0x8000, w, c, ((cpu->d[r]&0xffff)>>(c-1))&0x1);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    l = (cpu->d[r]>>c)|(cpu->d[r]<<(32-c));
    cpu_set_flags_rol(cpu, l&0x80000000, l, c, (cpu->d[r]>>(c-1))&0x1);
    cpu->d[r] = l;
    break;
  }
}

static void ror_m(struct cpu *cpu, WORD op)
{
  WORD d;

  d = ea_read_word(cpu, op&0x3f, 1);
  cpu_set_flags_rol(cpu, (d>>1)&0x8000, d>>1, 1, d&0x1);
  d = (d>>1)|(d<<15);
  ea_set_prefetch_before_write();
  ea_write_word(cpu, op&0x3f, d);

  ADD_CYCLE(8);
}

static void ror(struct cpu *cpu, WORD op)
{
  ENTER;

  if((op&0xc0) == 0xc0) {
    ror_m(cpu, op);
  } else if(op&0x20) {
    ror_r(cpu, op);
  } else {
    ror_i(cpu, op);
  }
  cpu_prefetch();
}

static struct cprint *ror_print(LONG addr, WORD op)
{
  int c,r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  if((op&0xc0) == 0xc0) {
    strcpy(ret->instr, "ROR.W");
    ea_print(ret, op&0x3f, 1);
  } else {
    r = op&0x7;
    c = (op&0xe00)>>9;
    switch((op&0xc0)>>6) {
    case 0:
      strcpy(ret->instr, "ROR.B");
      break;
    case 1:
      strcpy(ret->instr, "ROR.W");
      break;
    case 2:
      strcpy(ret->instr, "ROR.L");
      break;
    }
    if(op&0x20) {
      sprintf(ret->data, "D%d,D%d", c, r);
    } else {
      if(!c) c = 8;
      sprintf(ret->data, "#%d,D%d", c, r);
    }
  }

  return ret;
}

void ror_init(void *instr[], void *print[])
{
  int i,r;
  
  for(r=0;r<8;r++) {
    for(i=0;i<8;i++) {
      instr[0xe018|(r<<9)|i] = ror;
      instr[0xe038|(r<<9)|i] = ror;
      instr[0xe058|(r<<9)|i] = ror;
      instr[0xe078|(r<<9)|i] = ror;
      instr[0xe098|(r<<9)|i] = ror;
      instr[0xe0b8|(r<<9)|i] = ror;
      print[0xe018|(r<<9)|i] = ror_print;
      print[0xe038|(r<<9)|i] = ror_print;
      print[0xe058|(r<<9)|i] = ror_print;
      print[0xe078|(r<<9)|i] = ror_print;
      print[0xe098|(r<<9)|i] = ror_print;
      print[0xe0b8|(r<<9)|i] = ror_print;
    }
  }
  for(i=0;i<0x40;i++) {
    if(ea_valid(i, EA_INVALID_MEM)) {
      instr[0xe6c0|i] = ror;
      print[0xe6c0|i] = ror_print;
    }
  }
}
