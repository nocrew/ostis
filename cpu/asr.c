#include "common.h"
#include "cpu.h"
#include "ea.h"

static void asr_r(struct cpu *cpu, WORD op)
{
  int c,r;
  int mask;
  BYTE b;
  WORD w;
  LONG l;
  
  c = cpu->d[(op&0xe00)>>9]&63;
  r = (op&7);
  mask = 0;

  ADD_CYCLE(6+c);

  switch((op&0xc0)>>6) {
  case 0:
    if(cpu->d[r]&0x80) mask = (-1<<(8-c));
    b = ((cpu->d[r]&0xff)>>c)|mask;
    cpu_set_flags_lsr(cpu, b&0x80, b, c, ((cpu->d[r]&0xff)>>(c-1))&0x1);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    if(cpu->d[r]&0x8000) mask = (-1<<(16-c));
    w = ((cpu->d[r]&0xffff)>>c)|mask;
    cpu_set_flags_lsr(cpu, w&0x8000, w, c, ((cpu->d[r]&0xffff)>>(c-1))&0x1);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    if(cpu->d[r]&0x80000000) mask = (-1<<(32-c));
    l = (cpu->d[r]>>c)|mask;
    cpu_set_flags_lsr(cpu, l&0x80000000, l, c, (cpu->d[r]>>(c-1))&0x1);
    cpu->d[r] = l;
    break;
  }
}

static void asr_i(struct cpu *cpu, WORD op)
{
  int c,r;
  int mask;
  BYTE b;
  WORD w;
  LONG l;

  c = (op&0xe00)>>9;
  if(c == 0) c = 8;
  r = (op&7);
  mask = 0;
  
  ADD_CYCLE(6+c);

  switch((op&0xc0)>>6) {
  case 0:
    if(cpu->d[r]&0x80) mask = (-1<<(8-c));
    b = ((cpu->d[r]&0xff)>>c)|mask;
    cpu_set_flags_lsr(cpu, b&0x80, b, c, ((cpu->d[r]&0xff)>>(c-1))&0x1);
    cpu->d[r] = (cpu->d[r]&0xffffff00)|b;
    break;
  case 1:
    if(cpu->d[r]&0x8000) mask = (-1<<(16-c));
    w = ((cpu->d[r]&0xffff)>>c)|mask;
    cpu_set_flags_lsr(cpu, w&0x8000, w, c, ((cpu->d[r]&0xffff)>>(c-1))&0x1);
    cpu->d[r] = (cpu->d[r]&0xffff0000)|w;
    break;
  case 2:
    ADD_CYCLE(2);
    if(cpu->d[r]&0x80000000) mask = (-1<<(32-c));
    l = (cpu->d[r]>>c)|mask;
    cpu_set_flags_lsr(cpu, l&0x80000000, l, c, (cpu->d[r]>>(c-1))&0x1);
    cpu->d[r] = l;
    break;
  }
}

static void asr_m(struct cpu *cpu, WORD op)
{
  WORD d;

  d = ea_read_word(cpu, op&0x3f, 1);
  cpu_set_flags_lsr(cpu, (d>>1)&0x8000, d>>1, 1, d&0x1);
  if(d&0x8000) {
    d >>= 1;
    d |= 0x8000;
  } else {
    d >>= 1;
  }
  ea_write_word(cpu, op&0x3f, d);

  ADD_CYCLE(8);
}

static void asr(struct cpu *cpu, WORD op)
{
  ENTER;

  if((op&0xc0) == 0xc0) {
    asr_m(cpu, op);
  } else if(op&0x20) {
    asr_r(cpu, op);
  } else {
    asr_i(cpu, op);
  }
}

static struct cprint *asr_print(LONG addr, WORD op)
{
  int c,r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  if((op&0xc0) == 0xc0) {
    strcpy(ret->instr, "ASR.W");
    ea_print(ret, op&0x3f, 1);
  } else {
    r = op&0x7;
    c = (op&0xe00)>>9;
    switch((op&0xc0)>>6) {
    case 0:
      strcpy(ret->instr, "ASR.B");
      break;
    case 1:
      strcpy(ret->instr, "ASR.W");
      break;
    case 2:
      strcpy(ret->instr, "ASR.L");
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

void asr_init(void *instr[], void *print[])
{
  int i,r;
  
  for(r=0;r<8;r++) {
    for(i=0;i<8;i++) {
      instr[0xe000|(r<<9)|i] = asr;
      instr[0xe020|(r<<9)|i] = asr;
      instr[0xe040|(r<<9)|i] = asr;
      instr[0xe060|(r<<9)|i] = asr;
      instr[0xe080|(r<<9)|i] = asr;
      instr[0xe0a0|(r<<9)|i] = asr;
      print[0xe000|(r<<9)|i] = asr_print;
      print[0xe020|(r<<9)|i] = asr_print;
      print[0xe040|(r<<9)|i] = asr_print;
      print[0xe060|(r<<9)|i] = asr_print;
      print[0xe080|(r<<9)|i] = asr_print;
      print[0xe0a0|(r<<9)|i] = asr_print;
    }
  }
  for(i=0;i<0x40;i++) {
    instr[0xe0c0|i] = asr;
    print[0xe0c0|i] = asr_print;
  }
}
