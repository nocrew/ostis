#include "common.h"
#include "cpu.h"
#include "ea.h"

static void roxl_r(struct cpu *cpu, WORD op)
{
  int c,r,x;
  BYTE b;
  WORD w;
  LONG l;
  
  c = cpu->d[(op&0xe00)>>9]&63;
  r = (op&7);
  if(CHKX) x = 1; else x = 0;

  ADD_CYCLE(6+c);

  switch((op&0xc0)>>6) {
  case 0:
    b = ((cpu->d[r]&0xff)<<c)|((x<<c)>>1)|((cpu->d[r]&0xff)>>(9-c));
    cpu_set_flags_lsr(cpu, b&0x80, b, c, ((cpu->d[r]&0xff)<<(c-1))&0x80);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    w = ((cpu->d[r]&0xffff)<<c)|((x<<c)>>1)|((cpu->d[r]&0xffff)>>(17-c));
    cpu_set_flags_lsr(cpu, w&0x8000, w, c, ((cpu->d[r]&0xffff)<<(c-1))&0x8000);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    l = (cpu->d[r]<<c)|((x<<c)>>1)|(cpu->d[r]>>(33-c));
    cpu_set_flags_lsr(cpu, l&0x80000000, l, c, (cpu->d[r]<<(c-1))&0x80000000);
    cpu->d[r] = l;
    break;
  }
}

static void roxl_i(struct cpu *cpu, WORD op)
{
  int c,r,x;
  BYTE b;
  WORD w;
  LONG l;

  c = (op&0xe00)>>9;
  if(c == 0) c = 8;
  r = (op&7);
  if(CHKX) x = 1; else x = 0;

  ADD_CYCLE(6+c);

  switch((op&0xc0)>>6) {
  case 0:
    b = ((cpu->d[r]&0xff)<<c)|((x<<c)>>1)|((cpu->d[r]&0xff)>>(9-c));
    cpu_set_flags_lsr(cpu, b&0x80, b, c, ((cpu->d[r]&0xff)<<(c-1))&0x80);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    w = ((cpu->d[r]&0xffff)<<c)|((x<<c)>>1)|((cpu->d[r]&0xffff)>>(17-c));
    cpu_set_flags_lsr(cpu, w&0x8000, w, c, ((cpu->d[r]&0xffff)<<(c-1))&0x8000);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    l = (cpu->d[r]<<c)|((x<<c)>>1)|(cpu->d[r]>>(33-c));
    cpu_set_flags_lsr(cpu, l&0x80000000, l, c, (cpu->d[r]<<(c-1))&0x80000000);
    cpu->d[r] = l;
    break;
  }
}

static void roxl_m(struct cpu *cpu, WORD op)
{
  WORD d;
  int x;

  if(CHKX) x = 1; else x = 0;

  d = ea_read_word(cpu, op&0x3f, 1);
  cpu_set_flags_lsr(cpu, (d<<1)&0x8000, d<<1, 1, d&0x8000);
  d = (d<<1)|x|(d>>16);
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
