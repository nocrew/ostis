#include "common.h"
#include "cpu.h"
#include "ea.h"

static void lsl_r(struct cpu *cpu, WORD op)
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
    b = (cpu->d[r]&0xff)<<c;
    cpu_set_flags_lsr(cpu, b&0x80, b, c, ((cpu->d[r]&0xff)<<(c-1))&0x80);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    w = (cpu->d[r]&0xffff)<<c;
    cpu_set_flags_lsr(cpu, w&0x8000, w, c, ((cpu->d[r]&0xffff)<<(c-1))&0x8000);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    l = cpu->d[r]<<c;
    cpu_set_flags_lsr(cpu, l&0x80000000, l, c, (cpu->d[r]<<(c-1))&0x80000000);
    cpu->d[r] = l;
    break;
  }
}

static void lsl_i(struct cpu *cpu, WORD op)
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
    b = (cpu->d[r]&0xff)<<c;
    cpu_set_flags_lsr(cpu, b&0x80, b, c, ((cpu->d[r]&0xff)<<(c-1))&0x80);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    w = (cpu->d[r]&0xffff)<<c;
    cpu_set_flags_lsr(cpu, w&0x8000, w, c, ((cpu->d[r]&0xffff)<<(c-1))&0x8000);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    l = cpu->d[r]<<c;
    cpu_set_flags_lsr(cpu, l&0x80000000, l, c, (cpu->d[r]<<(c-1))&0x80000000);
    cpu->d[r] = l;
    break;
  }
}

static void lsl_m(struct cpu *cpu, WORD op)
{
  WORD d;

  d = ea_read_word(cpu, op&0x3f, 1);
  cpu_set_flags_lsr(cpu, (d<<1)&0x8000, d<<1, 1, d&0x8000);
  d <<= 1;
  ea_write_word(cpu, op&0x3f, d);

  ADD_CYCLE(8);
}

static void lsl(struct cpu *cpu, WORD op)
{
  ENTER;

  if((op&0xc0) == 0xc0) {
    lsl_m(cpu, op);
  } else if(op&0x20) {
    lsl_r(cpu, op);
  } else {
    lsl_i(cpu, op);
  }
}

static struct cprint *lsl_print(LONG addr, WORD op)
{
  int c,r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  if((op&0xc0) == 0xc0) {
    strcpy(ret->instr, "LSL.W");
    ea_print(ret, op&0x3f, 1);
  } else {
    r = op&0x7;
    c = (op&0xe00)>>9;
    switch((op&0xc0)>>6) {
    case 0:
      strcpy(ret->instr, "LSL.B");
      break;
    case 1:
      strcpy(ret->instr, "LSL.W");
      break;
    case 2:
      strcpy(ret->instr, "LSL.L");
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

void lsl_init(void *instr[], void *print[])
{
  int i,r;
  
  for(r=0;r<8;r++) {
    for(i=0;i<8;i++) {
      instr[0xe108|(r<<9)|i] = lsl;
      instr[0xe128|(r<<9)|i] = lsl;
      instr[0xe148|(r<<9)|i] = lsl;
      instr[0xe168|(r<<9)|i] = lsl;
      instr[0xe188|(r<<9)|i] = lsl;
      instr[0xe1a8|(r<<9)|i] = lsl;
      print[0xe108|(r<<9)|i] = lsl_print;
      print[0xe128|(r<<9)|i] = lsl_print;
      print[0xe148|(r<<9)|i] = lsl_print;
      print[0xe168|(r<<9)|i] = lsl_print;
      print[0xe188|(r<<9)|i] = lsl_print;
      print[0xe1a8|(r<<9)|i] = lsl_print;
    }
  }
  for(i=0;i<0x40;i++) {
    instr[0xe3c0|i] = lsl;
    print[0xe3c0|i] = lsl_print;
  }
}
