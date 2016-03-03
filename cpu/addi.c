#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"
#include "ea.h"

static void addi_b(struct cpu *cpu, WORD op)
{
  BYTE s,d,r;
  
  s = mmu_read_word(cpu->pc)&0xff;
  cpu->pc += 2;
  if(op&0x38) {
    ADD_CYCLE(12);
  } else {
    ADD_CYCLE(8);
  }
  d = ea_read_byte(cpu, op&0x3f, 1);
  r = d+s;
  ea_write_byte(cpu, op&0x3f, r);
  cpu_set_flags_add(cpu, s&0x80, d&0x80, r&0x80, r);
}

static void addi_w(struct cpu *cpu, WORD op)
{
  WORD s,d,r;
  
  s = mmu_read_word(cpu->pc);
  cpu->pc += 2;
  if(op&0x38) {
    ADD_CYCLE(12);
  } else {
    ADD_CYCLE(8);
  }
  d = ea_read_word(cpu, op&0x3f, 1);
  r = d+s;
  ea_write_word(cpu, op&0x3f, r);
  cpu_set_flags_add(cpu, s&0x8000, d&0x8000, r&0x8000, r);
}

static void addi_l(struct cpu *cpu, WORD op)
{
  LONG s,d,r;
  
  s = mmu_read_long(cpu->pc);
  cpu->pc += 4;
  if(op&0x38) {
    ADD_CYCLE(20);
  } else {
    ADD_CYCLE(16);
  }
  d = ea_read_long(cpu, op&0x3f, 1);
  r = d+s;
  ea_write_long(cpu, op&0x3f, r);
  cpu_set_flags_add(cpu, s&0x80000000, d&0x80000000, r&0x80000000, r);
}

static void addi(struct cpu *cpu, WORD op)
{
  ENTER;

  switch((op&0xc0)>>6) {
  case 0:
    addi_b(cpu, op);
    return;
  case 1:
    addi_w(cpu, op);
    return;
  case 2:
    addi_l(cpu, op);
    return;
  }
}

static struct cprint *addi_print(LONG addr, WORD op)
{
  BYTE b;
  WORD w;
  LONG l;
  int s,r;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  s = (op&0xc0)>>6;

  switch(s) {
  case 0:
    strcpy(ret->instr, "ADDI.B");
    b = mmu_read_word_print(addr+ret->size)&0xff;
    r = b;
    if(r&0x80) r |= 0xffffff00;
    if((r >= -128) && (r <= 127))
      sprintf(ret->data, "#%d,", r);
    else
      sprintf(ret->data, "#$%x,", r);
    ret->size += 2;
    break;
  case 1:
    strcpy(ret->instr, "ADDI.W");
    w = mmu_read_word_print(addr+ret->size);
    r = w;
    if(r&0x8000) r |= 0xffff0000;
    if((r >= -128) && (r <= 127))
      sprintf(ret->data, "#%d,", r);
    else
      sprintf(ret->data, "#$%x,", r);
    ret->size += 2;
    break;
  case 2:
    strcpy(ret->instr, "ADDI.L");
    l = mmu_read_long_print(addr+ret->size);
    r = (int)*((SLONG *)&l);
    if((r >= -128) && (r <= 127))
      sprintf(ret->data, "#%d,", r);
    else
      sprintf(ret->data, "#$%x,", r);
    ret->size += 4;
    break;
  }
  ea_print(ret, op&0x3f, s);

  return ret;
}

void addi_init(void *instr[], void *print[])
{
  int i;

  for(i=0;i<0xc0;i++) {
    if(ea_valid(i&0x3f, EA_INVALID_DST|EA_INVALID_A)) {
      instr[0x0600|i] = addi;
      print[0x0600|i] = addi_print;
    }
  }
}

