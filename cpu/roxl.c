#include "common.h"
#include "cpu.h"
#include "ea.h"

static BYTE roxl_byte_one(BYTE value, int x, int *carry)
{
  *carry = 0;
  *carry = (value&0x80)>>7;
  value = value<<1;
  value |= x;
  return value;
}

static BYTE roxl_byte_count(BYTE value, int count, int x, int *carry)
{
  int i;

  if(count == 0) {
    *carry = x;
    return value;
  }
    
  for(i=0;i<count;i++) {
    value = roxl_byte_one(value, x, carry);
    x = *carry;
  }
  return value;
}

static WORD roxl_word_one(WORD value, int x, int *carry)
{
  *carry = 0;
  *carry = (value&0x8000)>>15;
  value = value<<1;
  value |= x;
  return value;
}

static WORD roxl_word_count(WORD value, int count, int x, int *carry)
{
  int i;
  
  if(count == 0) {
    *carry = x;
    return value;
  }
    
  for(i=0;i<count;i++) {
    value = roxl_word_one(value, x, carry);
    x = *carry;
  }
  return value;
}

static LONG roxl_long_one(LONG value, int x, int *carry)
{
  *carry = 0;
  *carry = (value&0x80000000)>>31;
  value = value<<1;
  value |= x;
  return value;
}

static LONG roxl_long_count(LONG value, int count, int x, int *carry)
{
  int i;
  
  if(count == 0) {
    *carry = x;
    return value;
  }
    
  for(i=0;i<count;i++) {
    value = roxl_long_one(value, x, carry);
    x = *carry;
  }
  return value;
}

static void roxl_set_flags(struct cpu *cpu, int xflag, int cflag, int nflag, int zflag)
{
  CLRV;
  if(xflag) SETX; else CLRX;
  if(cflag) SETC; else CLRC;
  if(nflag) SETN; else CLRN;
  if(zflag) SETZ; else CLRZ;
}

static void roxl_r(struct cpu *cpu, WORD op)
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
    b = roxl_byte_count(cpu->d[r]&0xff, c, x, &carry);
    roxl_set_flags(cpu, carry, carry, b&80, !b);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    w = roxl_word_count(cpu->d[r]&0xffff, c, x, &carry);
    roxl_set_flags(cpu, carry, carry, w&8000, !w);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    l = roxl_long_count(cpu->d[r], c, x, &carry);
    roxl_set_flags(cpu, carry, carry, l&80000000, !l);
    cpu->d[r] = l;
    break;
  }
}

static void roxl_i(struct cpu *cpu, WORD op)
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
    b = roxl_byte_count(cpu->d[r]&0xff, c, x, &carry);
    roxl_set_flags(cpu, carry, carry, b&80, !b);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    w = roxl_word_count(cpu->d[r]&0xffff, c, x, &carry);
    roxl_set_flags(cpu, carry, carry, w&8000, !w);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    l = roxl_long_count(cpu->d[r], c, x, &carry);
    roxl_set_flags(cpu, carry, carry, l&80000000, !l);
    cpu->d[r] = l;
    break;
  }
}

static void roxl_m(struct cpu *cpu, WORD op)
{
  WORD d;
  int x,carry;

  if(CHKX) x = 1; else x = 0;

  d = ea_read_word(cpu, op&0x3f, 1);
  d = roxl_word_count(d, 1, x, &carry);
  roxl_set_flags(cpu, carry, carry, d&8000, !d);
  ea_write_word(cpu, op&0x3f, d);

  ADD_CYCLE(8);
}

static void roxl(struct cpu *cpu, WORD op)
{
  ENTER;

  if((op&0xc0) == 0xc0) {
    roxl_m(cpu, op);
  } else if(op&0x20) {
    roxl_r(cpu, op);
  } else {
    roxl_i(cpu, op);
  }
}

static struct cprint *roxl_print(LONG addr, WORD op)
{
  int c,r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  if((op&0xc0) == 0xc0) {
    strcpy(ret->instr, "ROXL.W");
    ea_print(ret, op&0x3f, 1);
  } else {
    r = op&0x7;
    c = (op&0xe00)>>9;
    switch((op&0xc0)>>6) {
    case 0:
      strcpy(ret->instr, "ROXL.B");
      break;
    case 1:
      strcpy(ret->instr, "ROXL.W");
      break;
    case 2:
      strcpy(ret->instr, "ROXL.L");
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

void roxl_init(void *instr[], void *print[])
{
  int i,r;
  
  for(r=0;r<8;r++) {
    for(i=0;i<8;i++) {
      instr[0xe110|(r<<9)|i] = roxl;
      instr[0xe130|(r<<9)|i] = roxl;
      instr[0xe150|(r<<9)|i] = roxl;
      instr[0xe170|(r<<9)|i] = roxl;
      instr[0xe190|(r<<9)|i] = roxl;
      instr[0xe1b0|(r<<9)|i] = roxl;
      print[0xe110|(r<<9)|i] = roxl_print;
      print[0xe130|(r<<9)|i] = roxl_print;
      print[0xe150|(r<<9)|i] = roxl_print;
      print[0xe170|(r<<9)|i] = roxl_print;
      print[0xe190|(r<<9)|i] = roxl_print;
      print[0xe1b0|(r<<9)|i] = roxl_print;
    }
  }
  for(i=0;i<0x40;i++) {
    if(ea_valid(i, EA_INVALID_MEM)) {
      instr[0xe5c0|i] = roxl;
      print[0xe5c0|i] = roxl_print;
    }
  }
}
