#include "common.h"
#include "cpu.h"
#include "ea.h"

static void rol_r(struct cpu *cpu, WORD op)
{
  int c,r;
  BYTE b;
  WORD w;
  LONG l;
  
  c = cpu->d[(op&0xe00)>>9]&63;
  r = (op&7);

  ADD_CYCLE(6+c);

  switch((op&0xc0)>>6) {
  case 0:
    b = ((cpu->d[r]&0xff)<<c)|((cpu->d[r]&0xff)>>(8-c));
    cpu_set_flags_rol(cpu, b&0x80, b, c, ((cpu->d[r]&0xff)<<(c-1))&0x80);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    w = ((cpu->d[r]&0xffff)<<c)|((cpu->d[r]&0xffff)>>(16-c));
    cpu_set_flags_rol(cpu, w&0x8000, w, c, ((cpu->d[r]&0xffff)<<(c-1))&0x8000);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    l = (cpu->d[r]<<c)|(cpu->d[r]>>(32-c));
    cpu_set_flags_rol(cpu, l&0x80000000, l, c, (cpu->d[r]<<(c-1))&0x80000000);
    cpu->d[r] = l;
    break;
  }
}

static void rol_i(struct cpu *cpu, WORD op)
{
  int c,r;
  BYTE b;
  WORD w;
  LONG l;

  c = (op&0xe00)>>9;
  if(c == 0) c = 8;
  r = (op&7);
  
  ADD_CYCLE(6+c);

  switch((op&0xc0)>>6) {
  case 0:
    b = ((cpu->d[r]&0xff)<<c)|((cpu->d[r]&0xff)>>(8-c));
    cpu_set_flags_rol(cpu, b&0x80, b, c, ((cpu->d[r]&0xff)<<(c-1))&0x80);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    w = ((cpu->d[r]&0xffff)<<c)|((cpu->d[r]&0xffff)>>(16-c));
    cpu_set_flags_rol(cpu, w&0x8000, w, c, ((cpu->d[r]&0xffff)<<(c-1))&0x8000);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    l = (cpu->d[r]<<c)|(cpu->d[r]>>(32-c));
    cpu_set_flags_rol(cpu, l&0x80000000, l, c, (cpu->d[r]<<(c-1))&0x80000000);
    cpu->d[r] = l;
    break;
  }
}

static void rol_m(struct cpu *cpu, WORD op)
{
  WORD d;

  d = ea_read_word(cpu, op&0x3f, 1);
  cpu_set_flags_rol(cpu, (d<<1)&0x8000, d<<1, 1, d&0x8000);
  d = (d<<1)|(d>>15);
  ea_write_word(cpu, op&0x3f, d);

  ADD_CYCLE(8);
}

static void rol(struct cpu *cpu, WORD op)
{
  ENTER;

  if((op&0xc0) == 0xc0) {
    rol_m(cpu, op);
  } else if(op&0x20) {
    rol_r(cpu, op);
  } else {
    rol_i(cpu, op);
  }
}

static struct cprint *rol_print(LONG addr, WORD op)
{
  int c,r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  if((op&0xc0) == 0xc0) {
    strcpy(ret->instr, "ROL.W");
    ea_print(ret, op&0x3f, 1);
  } else {
    r = op&0x7;
    c = (op&0xe00)>>9;
    switch((op&0xc0)>>6) {
    case 0:
      strcpy(ret->instr, "ROL.B");
      break;
    case 1:
      strcpy(ret->instr, "ROL.W");
      break;
    case 2:
      strcpy(ret->instr, "ROL.L");
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

void rol_init(void *instr[], void *print[])
{
  int i,r;
  
  for(r=0;r<8;r++) {
    for(i=0;i<8;i++) {
      instr[0xe118|(r<<9)|i] = rol;
      instr[0xe138|(r<<9)|i] = rol;
      instr[0xe158|(r<<9)|i] = rol;
      instr[0xe178|(r<<9)|i] = rol;
      instr[0xe198|(r<<9)|i] = rol;
      instr[0xe1b8|(r<<9)|i] = rol;
      print[0xe118|(r<<9)|i] = rol_print;
      print[0xe138|(r<<9)|i] = rol_print;
      print[0xe158|(r<<9)|i] = rol_print;
      print[0xe178|(r<<9)|i] = rol_print;
      print[0xe198|(r<<9)|i] = rol_print;
      print[0xe1b8|(r<<9)|i] = rol_print;
    }
  }
  for(i=0;i<0x40;i++) {
    instr[0xe7c0|i] = rol;
    print[0xe7c0|i] = rol_print;
  }
}
