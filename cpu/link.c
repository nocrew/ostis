#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"

static void link(struct cpu *cpu, WORD op)
{
  int r;
  LONG d;

  ENTER;

  d = bus_read_word(cpu->pc);
  cpu->pc += 2;
  if(d&0x8000) d |= 0xffff0000;
  
  r = op&0x7;

  cpu->a[7] -= 4;
  cpu_prefetch();
  bus_write_long(cpu->a[7], cpu->a[r]);
  cpu->a[r] = cpu->a[7];
  cpu->a[7] += d;

  ADD_CYCLE(16);
}

static struct cprint *link_print(LONG addr, WORD op)
{
  int r,d;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  d = bus_read_word_print(addr+ret->size);
  if(d&0x8000) d |= 0xffff0000;
  ret->size += 2;
  
  r = op&0x7;

  strcpy(ret->instr, "LINK");
  sprintf(ret->data, "A%d,#%d", r, d);
  
  return ret;
}

void link_init(void *instr[], void *print[])
{
  int r;
  for(r=0;r<8;r++) {
    instr[0x4e50|r] = link;
    print[0x4e50|r] = link_print;
  }
}
