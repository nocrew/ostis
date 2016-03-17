#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"
#include "ea.h"

static void movem_w(struct cpu *cpu, WORD op, int rmask)
{
  WORD d;
  LONG a;
  int i,savecycle,rev,inc,cnt;

  savecycle = cpu->icycle;
  a = ea_get_addr(cpu, op&0x3f);
  cnt = 0;
  if(((op&0x38)>>3) == 4) {
    rev = 1;
    a -= 2;
    inc = 1;
  } else {
    if(((op&0x38)>>3) == 3) {
      inc = 1;
    } else {
      inc = 0;
    }
    rev = 0;
  }
  if(inc && op&0x400) {
    int ea_reg = op&0x7;
    if(rev) {
      if(rmask & (1<<(15-(ea_reg+8)))) {
        inc = 0;
      }
    } else {
      if(rmask & (1<<(ea_reg+8))) {
        inc = 0;
      }
    }
  }
  cpu->icycle = savecycle;

  if(op&0x400) {
    for(i=0;i<8;i++) {
      if(rmask&(1<<i)) {
	d = bus_read_word(a+cnt*2);
	if(d&0x8000) d |= 0xffff0000;
	cpu->d[i] = d;
	cnt++;
	cpu_do_cycle(cpu->icycle+4);
	cpu->icycle = 0;
      }
    }
    for(i=0;i<8;i++) {
      if(rmask&(1<<(i+8))) {
	d = bus_read_word(a+cnt*2);
	if(d&0x8000) d |= 0xffff0000;
	cpu->a[i] = d;
	cnt++;
	cpu_do_cycle(cpu->icycle+4);
	cpu->icycle = 0;
      }
    }
    if(inc) cpu->a[op&0x7]+=cnt*2;
  } else {
    if(rev) {
      for(i=7;i>=0;i--) {
	if(rmask&(1<<(15-(i+8)))) {
	  d = cpu->a[i]&0xffff;
          cpu_prefetch();
	  bus_write_word(a-cnt*2, d);
	  cnt++;
	  cpu_do_cycle(cpu->icycle+4);
	  cpu->icycle = 0;
	}
      }
      for(i=7;i>=0;i--) {
	if(rmask&(1<<(15-i))) {
	  d = cpu->d[i]&0xffff;
          cpu_prefetch();
	  bus_write_word(a-cnt*2, d);
	  cnt++;
	  cpu_do_cycle(cpu->icycle+4);
	  cpu->icycle = 0;
	}
      }
      if(inc) cpu->a[op&0x7]-=cnt*2;
    } else {
      for(i=0;i<8;i++) {
	if(rmask&(1<<i)) {
	  d = cpu->d[i]&0xffff;
          cpu_prefetch();
	  bus_write_word(a+cnt*2, d);
	  cnt++;
	  cpu_do_cycle(cpu->icycle+4);
	  cpu->icycle = 0;
	}
      }
      for(i=0;i<8;i++) {
	if(rmask&(1<<(i+8))) {
	  d = cpu->a[i]&0xffff;
          cpu_prefetch();
	  bus_write_word(a+cnt*2, d);
	  cnt++;
	  cpu_do_cycle(cpu->icycle+4);
	  cpu->icycle = 0;
	}
      }
      if(inc) cpu->a[op&0x7]+=cnt*2;
    }
  }
}

static void movem_l(struct cpu *cpu, WORD op, int rmask)
{
  LONG d;
  LONG a;
  int i,savecycle,rev,inc,cnt;

  savecycle = cpu->icycle;
  a = ea_get_addr(cpu, op&0x3f);
  cnt = 0;
  if(((op&0x38)>>3) == 4) {
    rev = 1;
    a -= 4;
    inc = 1;
  } else {
    if(((op&0x38)>>3) == 3) {
      inc = 1;
    } else {
      inc = 0;
    }
    rev = 0;
  }
  if(inc && op&0x400) {
    int ea_reg = op&0x7;
    if(rev) {
      if(rmask & (1<<(15-(ea_reg+8)))) {
        inc = 0;
      }
    } else {
      if(rmask & (1<<(ea_reg+8))) {
        inc = 0;
      }
    }
  }
  cpu->icycle = savecycle;
  
  if(op&0x400) {
    for(i=0;i<8;i++) {
      if(rmask&(1<<i)) {
	d = bus_read_long(a+cnt*4);
	cpu->d[i] = d;
	cnt++;
	cpu_do_cycle(cpu->icycle+8);
	cpu->icycle = 0;
      }
    }
    for(i=0;i<8;i++) {
      if(rmask&(1<<(i+8))) {
	d = bus_read_long(a+cnt*4);
	cpu->a[i] = d;
	cnt++;
	cpu_do_cycle(cpu->icycle+8);
	cpu->icycle = 0;
      }
    }
    if(inc) cpu->a[op&0x7]+=cnt*4;
  } else {
    if(rev) {
      for(i=7;i>=0;i--) {
	if(rmask&(1<<(15-(i+8)))) {
	  d = cpu->a[i];
          cpu_prefetch();
	  bus_write_long(a-cnt*4, d);
	  cnt++;
	  cpu_do_cycle(cpu->icycle+8);
	  cpu->icycle = 0;
	}
      }
      for(i=7;i>=0;i--) {
	if(rmask&(1<<(15-i))) {
	  d = cpu->d[i];
          cpu_prefetch();
	  bus_write_long(a-cnt*4, d);
	  cnt++;
	  cpu_do_cycle(cpu->icycle+8);
	  cpu->icycle = 0;
	}
      }
      if(inc) cpu->a[op&0x7]-=cnt*4;
    } else {
      for(i=0;i<8;i++) {
	if(rmask&(1<<i)) {
	  d = cpu->d[i];
          cpu_prefetch();
	  bus_write_long(a+cnt*4, d);
	  cnt++;
	  cpu_do_cycle(cpu->icycle+8);
	  cpu->icycle = 0;
	}
      }
      for(i=0;i<8;i++) {
	if(rmask&(1<<(i+8))) {
	  d = cpu->a[i];
          cpu_prefetch();
	  bus_write_long(a+cnt*4, d);
	  cnt++;
	  cpu_do_cycle(cpu->icycle+8);
	  cpu->icycle = 0;
	}
      }
      if(inc) cpu->a[op&0x7]+=cnt*4;
    }
  }
}

static void movem(struct cpu *cpu, WORD op)
{
  int rmask;

  ENTER;

  rmask = bus_read_word(cpu->pc);
  cpu->pc += 2;

  switch((op&0x38)>>3) {
  case 2:
    ADD_CYCLE(8);
    break;
  case 3:
    ADD_CYCLE(8);
    break;
  case 4:
    ADD_CYCLE(8);
    break;
  case 5:
    ADD_CYCLE(12);
    break;
  case 6:
    ADD_CYCLE(14);
    break;
  case 7:
    if((op&7) == 0) {
      ADD_CYCLE(12);
    } else if((op&7) == 1) {
      ADD_CYCLE(16);
    } else if((op&7) == 2) {
      ADD_CYCLE(12);
    } else if((op&7) == 3) {
      ADD_CYCLE(14);
    }
    break;
  }

  if(op&0x400) {
    ADD_CYCLE(4);
  }
  
  if(op&0x40) {
    movem_l(cpu, op, rmask);
  } else {
    movem_w(cpu, op, rmask);
  }
}

static void rmask_print(struct cprint *cprint, int rmask)
{
  int d,a;
  int first,end,last,sep;

  first = 1;
  sep = 0;
  end = 0;
  last = 0;

  for(d=0;d<8;d++) {
    if(rmask&(1<<d)) {
      if(!last) {
        if(sep) strcat(cprint->data, "/");
        sprintf(cprint->data, "%sD%d", cprint->data, d);
        first = 0;
        end = 0;
      } else {
        end = d;
      }
      last = 1;
    } else {
      if(last) {
        if(end) sprintf(cprint->data, "%s-D%d", cprint->data, end);
      }
      if(!first) sep = 1;
      last = 0;
    }
  }
  if(end == 7) strcat(cprint->data, "-D7");

  last = 0;
  end = 0;
  if(!first) sep = 1;

  for(a=0;a<8;a++) {
    if(rmask&(1<<(a+8))) {
      if(!last) {
        if(sep) strcat(cprint->data, "/");
        sprintf(cprint->data, "%sA%d", cprint->data, a);
        first = 0;
        end = 0;
      } else {
        end = a;
      }
      last = 1;
    } else {
      if(last) {
        if(end) sprintf(cprint->data, "%s-A%d", cprint->data, end);
      }
      if(!first) sep = 1;
      last = 0;
    }
  }
  if(end == 7) strcat(cprint->data, "-A7");
}

static int rmask_reverse(int rmask)
{
  int i,t;

  t = 0;

  for(i=0;i<16;i++) {
    t<<=1;
    t |= rmask&1;
    rmask>>=1;
  }
  return t;
}

static struct cprint *movem_print(LONG addr, WORD op)
{
  int rmask;
  struct cprint *ret;
  
  ret = cprint_alloc(addr);

  rmask = bus_read_word_print(addr+ret->size);
  ret->size += 2;
  
  if(((op&0x38)>>3) == 4) {
    rmask = rmask_reverse(rmask);
  }

  switch(op&0x40) {
  case 0:
    strcpy(ret->instr, "MOVEM.W");
    break;
  case 0x40:
    strcpy(ret->instr, "MOVEM.L");
    break;
  }

  if(op&0x400) {
    ea_print(ret, op&0x3f, 1);
    strcat(ret->data, ",");
    rmask_print(ret, rmask);
  } else {
    rmask_print(ret, rmask);
    strcat(ret->data, ",");
    ea_print(ret, op&0x3f, 1);
  }

  return ret;
}

void movem_init(void *instr[], void *print[])
{
  int i;
  
  for(i=0;i<0x40;i++) {
    if(ea_valid(i, EA_INVALID_MEM|EA_INVALID_INC)) {
      instr[0x4880|i] = movem;
      instr[0x48c0|i] = movem;
      print[0x4880|i] = movem_print;
      print[0x48c0|i] = movem_print;
    }
    if(ea_valid(i, EA_INVALID_I|EA_INVALID_A|EA_INVALID_D|EA_INVALID_DEC)) {
      instr[0x4c80|i] = movem;
      instr[0x4cc0|i] = movem;
      print[0x4c80|i] = movem_print;
      print[0x4cc0|i] = movem_print;
    }
  }
}
