#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "ea.h"

static BYTE roxr_byte_one(BYTE value, int x, int *carry)
{
  *carry = 0;
  *carry = value&0x1;
  value = value>>1;
  value |= x<<7;
  return value;
}

static BYTE roxr_byte_count(BYTE value, int count, int x, int *carry)
{
  int i;

  if(count == 0) {
    *carry = x;
    return value;
  }
    
  for(i=0;i<count;i++) {
    value = roxr_byte_one(value, x, carry);
    x = *carry;
  }
  return value;
}

static WORD roxr_word_one(WORD value, int x, int *carry)
{
  *carry = 0;
  *carry = value&0x1;
  value = value>>1;
  value |= x<<15;
  return value;
}

static WORD roxr_word_count(WORD value, int count, int x, int *carry)
{
  int i;
  
  if(count == 0) {
    *carry = x;
    return value;
  }
    
  for(i=0;i<count;i++) {
    value = roxr_word_one(value, x, carry);
    x = *carry;
  }
  return value;
}

static LONG roxr_long_one(LONG value, int x, int *carry)
{
  *carry = 0;
  *carry = value&0x1;
  value = value>>1;
  value |= x<<31;
  return value;
}

static LONG roxr_long_count(LONG value, int count, int x, int *carry)
{
  int i;
  
  if(count == 0) {
    *carry = x;
    return value;
  }
    
  for(i=0;i<count;i++) {
    value = roxr_long_one(value, x, carry);
    x = *carry;
  }
  return value;
}

static void roxr_set_flags(struct cpu *cpu, int xflag, int cflag, int nflag, int zflag)
{
  CLRV;
  if(xflag) SETX; else CLRX;
  if(cflag) SETC; else CLRC;
  if(nflag) SETN; else CLRN;
  if(zflag) SETZ; else CLRZ;
}

static void roxr_r(struct cpu *cpu, WORD op)
{
  int c,r,x,carry;
  BYTE b;
  WORD w;
  LONG l;
  
  c = cpu->d[(op&0xe00)>>9]&63;
  r = (op&7);
  if(CHKX) x = 1; else x = 0;

  ADD_CYCLE(6+c*2);

  switch((op&0xc0)>>6) {
  case 0:
    b = roxr_byte_count(cpu->d[r]&0xff, c, x, &carry);
    roxr_set_flags(cpu, carry, carry, b&80, !b);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    w = roxr_word_count(cpu->d[r]&0xffff, c, x, &carry);
    roxr_set_flags(cpu, carry, carry, w&8000, !w);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    l = roxr_long_count(cpu->d[r], c, x, &carry);
    roxr_set_flags(cpu, carry, carry, l&80000000, !l);
    cpu->d[r] = l;
    break;
  }
}

static void roxr_i(struct cpu *cpu, WORD op)
{
  int c,r,x,carry;
  BYTE b;
  WORD w;
  LONG l;

  c = (op&0xe00)>>9;
  if(c == 0) c = 8;
  r = (op&7);
  if(CHKX) x = 1; else x = 0;

  ADD_CYCLE(6+c*2);

  switch((op&0xc0)>>6) {
  case 0:
    b = roxr_byte_count(cpu->d[r]&0xff, c, x, &carry);
    roxr_set_flags(cpu, carry, carry, b&80, !b);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    w = roxr_word_count(cpu->d[r]&0xffff, c, x, &carry);
    roxr_set_flags(cpu, carry, carry, w&8000, !w);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    l = roxr_long_count(cpu->d[r], c, x, &carry);
    roxr_set_flags(cpu, carry, carry, l&80000000, !l);
    cpu->d[r] = l;
    break;
  }
}

static void roxr_m(struct cpu *cpu, WORD op)
{
  WORD d;
  int x,carry;

  if(CHKX) x = 1; else x = 0;

  d = ea_read_word(cpu, op&0x3f, 1);
  d = roxr_word_count(d, 1, x, &carry);
  roxr_set_flags(cpu, carry, carry, d&8000, !d);
  ea_write_word(cpu, op&0x3f, d);

  ADD_CYCLE(8);
}

static void roxr(struct cpu *cpu, WORD op)
{
  ENTER;

  if((op&0xc0) == 0xc0) {
    roxr_m(cpu, op);
  } else if(op&0x20) {
    roxr_r(cpu, op);
  } else {
    roxr_i(cpu, op);
  }
}

static struct cprint *roxr_print(LONG addr, WORD op)
{
  int c,r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  if((op&0xc0) == 0xc0) {
    strcpy(ret->instr, "ROXR.W");
    ea_print(ret, op&0x3f, 1);
  } else {
    r = op&0x7;
    c = (op&0xe00)>>9;
    switch((op&0xc0)>>6) {
    case 0:
      strcpy(ret->instr, "ROXR.B");
      break;
    case 1:
      strcpy(ret->instr, "ROXR.W");
      break;
    case 2:
      strcpy(ret->instr, "ROXR.L");
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

void roxr_init(void *instr[], void *print[])
{
  int i,r;
  
  for(r=0;r<8;r++) {
    for(i=0;i<8;i++) {
      instr[0xe010|(r<<9)|i] = roxr;
      instr[0xe030|(r<<9)|i] = roxr;
      instr[0xe050|(r<<9)|i] = roxr;
      instr[0xe070|(r<<9)|i] = roxr;
      instr[0xe090|(r<<9)|i] = roxr;
      instr[0xe0b0|(r<<9)|i] = roxr;
      print[0xe010|(r<<9)|i] = roxr_print;
      print[0xe030|(r<<9)|i] = roxr_print;
      print[0xe050|(r<<9)|i] = roxr_print;
      print[0xe070|(r<<9)|i] = roxr_print;
      print[0xe090|(r<<9)|i] = roxr_print;
      print[0xe0b0|(r<<9)|i] = roxr_print;
    }
  }
  for(i=0;i<0x40;i++) {
    if(ea_valid(i, EA_INVALID_MEM)) {
      instr[0xe4c0|i] = roxr;
      print[0xe4c0|i] = roxr_print;
    }
  }
}
