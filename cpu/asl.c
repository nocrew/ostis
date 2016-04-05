#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

static void asl_r(struct cpu *cpu, WORD op)
{
  int c,r;
  int vbit;
  BYTE b;
  WORD w;
  LONG l;
  
  c = cpu->d[(op&0xe00)>>9]&63;
  r = (op&7);

  ADD_CYCLE(6+c*2);

  switch((op&0xc0)>>6) {
  case 0:
    b = (cpu->d[r]&0xff)<<c;
    if(cpu->d[r]&0x80) {
      vbit = (-1<<(7-c))&0x7f;
      vbit = (cpu->d[r]&vbit)^vbit;
    } else {
      vbit = (-1<<(7-c))&0x7f;
      vbit = (~(cpu->d[r])&vbit)^vbit;
    }
    cpu_set_flags_asl(cpu, b&0x80, b, c, ((cpu->d[r]&0xff)<<(c-1))&0x80, vbit);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    w = (cpu->d[r]&0xffff)<<c;
    if(cpu->d[r]&0x8000) {
      vbit = (-1<<(7-c))&0x7fff;
      vbit = (cpu->d[r]&vbit)^vbit;
    } else {
      vbit = (-1<<(7-c))&0x7fff;
      vbit = (~(cpu->d[r])&vbit)^vbit;
    }
    cpu_set_flags_asl(cpu, w&0x8000, w, c, ((cpu->d[r]&0xffff)<<(c-1))&0x8000, vbit);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    l = cpu->d[r]<<c;
    if(cpu->d[r]&0x80000000) {
      vbit = (-1<<(7-c))&0x7fffffff;
      vbit = (cpu->d[r]&vbit)^vbit;
    } else {
      vbit = (-1<<(7-c))&0x7fffffff;
      vbit = (~(cpu->d[r])&vbit)^vbit;
    }
    cpu_set_flags_asl(cpu, l&0x80000000, l, c, (cpu->d[r]<<(c-1))&0x80000000, vbit);
    cpu->d[r] = l;
    break;
  }
}

static void asl_i(struct cpu *cpu, WORD op)
{
  int c,r;
  int vbit;
  BYTE b;
  WORD w;
  LONG l;

  c = (op&0xe00)>>9;
  if(c == 0) c = 8;
  r = (op&7);
  
  ADD_CYCLE(6+c*2);

  switch((op&0xc0)>>6) {
  case 0:
    b = (cpu->d[r]&0xff)<<c;
    if(cpu->d[r]&0x80) {
      vbit = (-1<<(7-c))&0x7f;
      vbit = (cpu->d[r]&vbit)^vbit;
    } else {
      vbit = (-1<<(7-c))&0x7f;
      vbit = (~(cpu->d[r])&vbit)^vbit;
    }
    cpu_set_flags_asl(cpu, b&0x80, b, c, ((cpu->d[r]&0xff)<<(c-1))&0x80, vbit);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    w = (cpu->d[r]&0xffff)<<c;
    if(cpu->d[r]&0x8000) {
      vbit = (-1<<(7-c))&0x7fff;
      vbit = (cpu->d[r]&vbit)^vbit;
    } else {
      vbit = (-1<<(7-c))&0x7fff;
      vbit = (~(cpu->d[r])&vbit)^vbit;
    }
    cpu_set_flags_asl(cpu, w&0x8000, w, c, ((cpu->d[r]&0xffff)<<(c-1))&0x8000, vbit);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    l = cpu->d[r]<<c;
    if(cpu->d[r]&0x80000000) {
      vbit = (-1<<(7-c))&0x7fffffff;
      vbit = (cpu->d[r]&vbit)^vbit;
    } else {
      vbit = (-1<<(7-c))&0x7fffffff;
      vbit = (~(cpu->d[r])&vbit)^vbit;
    }
    cpu_set_flags_asl(cpu, l&0x80000000, l, c, (cpu->d[r]<<(c-1))&0x80000000, vbit);
    cpu->d[r] = l;
    break;
  }
}

static void asl_m(struct cpu *cpu, WORD op)
{
  WORD d;

  d = ea_read_word(cpu, op&0x3f, 1);
  cpu_set_flags_asl(cpu, (d<<1)&0x8000, d<<1, 1, d&0x8000, ((d<<1)^d)&0x8000);
  d <<= 1;
  ea_set_prefetch_before_write();
  ea_write_word(cpu, op&0x3f, d);

  ADD_CYCLE(8);
}

static void asl(struct cpu *cpu, WORD op)
{
  ENTER;

  if((op&0xc0) == 0xc0) {
    asl_m(cpu, op);
  } else if(op&0x20) {
    asl_r(cpu, op);
  } else {
    asl_i(cpu, op);
  }
  cpu_prefetch();
}

static struct cprint *asl_print(LONG addr, WORD op)
{
  int c,r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  if((op&0xc0) == 0xc0) {
    strcpy(ret->instr, "ASL.W");
    ea_print(ret, op&0x3f, 1);
  } else {
    r = op&0x7;
    c = (op&0xe00)>>9;
    switch((op&0xc0)>>6) {
    case 0:
      strcpy(ret->instr, "ASL.B");
      break;
    case 1:
      strcpy(ret->instr, "ASL.W");
      break;
    case 2:
      strcpy(ret->instr, "ASL.L");
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

void asl_init(void *instr[], void *print[])
{
  int i,r;
  
  for(r=0;r<8;r++) {
    for(i=0;i<8;i++) {
      instr[0xe100|(r<<9)|i] = asl;
      instr[0xe120|(r<<9)|i] = asl;
      instr[0xe140|(r<<9)|i] = asl;
      instr[0xe160|(r<<9)|i] = asl;
      instr[0xe180|(r<<9)|i] = asl;
      instr[0xe1a0|(r<<9)|i] = asl;
      print[0xe100|(r<<9)|i] = asl_print;
      print[0xe120|(r<<9)|i] = asl_print;
      print[0xe140|(r<<9)|i] = asl_print;
      print[0xe160|(r<<9)|i] = asl_print;
      print[0xe180|(r<<9)|i] = asl_print;
      print[0xe1a0|(r<<9)|i] = asl_print;
    }
  }
  for(i=0;i<0x40;i++) {
    if(ea_valid(i, EA_INVALID_MEM)) {
      instr[0xe1c0|i] = asl;
      print[0xe1c0|i] = asl_print;
    }
  }
}




