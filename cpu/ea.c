#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"
#include "ea.h"

static int rmw = 0;
static int ea_prefetch_before_write = 0;

static BYTE ea_read_000_b(struct cpu *cpu, int reg)
{
  return cpu->d[reg]&MASK_B;
}

static WORD ea_read_000_w(struct cpu *cpu, int reg)
{
  return cpu->d[reg]&MASK_W;
}

static LONG ea_read_000_l(struct cpu *cpu, int reg)
{
  return cpu->d[reg]&MASK_L;
}

static void ea_write_000_b(struct cpu *cpu, int reg, BYTE data)
{
  cpu->d[reg] = (cpu->d[reg]&0xffffff00)|data;
}

static void ea_write_000_w(struct cpu *cpu, int reg, WORD data)
{
  cpu->d[reg] = (cpu->d[reg]&0xffff0000)|data;
}

static void ea_write_000_l(struct cpu *cpu, int reg, LONG data)
{
  cpu->d[reg] = data;
}

static BYTE ea_read_001_b(struct cpu *cpu, int reg)
{
  return cpu->a[reg]&MASK_B;
}

static WORD ea_read_001_w(struct cpu *cpu, int reg)
{
  return cpu->a[reg]&MASK_W;
}

static LONG ea_read_001_l(struct cpu *cpu, int reg)
{
  return cpu->a[reg]&MASK_L;
}

static LONG ea_addr_010(struct cpu *cpu, int reg)
{
  return cpu->a[reg];
}

static LONG ea_addr_011(struct cpu *cpu, int reg, int size)
{
  if(rmw) {
    if((size == 1) && (reg == 7)) size = 2;
    return cpu->a[reg];
  } else {
    if((size == 1) && (reg == 7)) size = 2; /* SP should get mad, not even */
    cpu->a[reg]+=size;
    return cpu->a[reg]-size;
  }
}

static LONG ea_addr_100(struct cpu *cpu, int reg, int size)
{
  if(rmw) {
    if((size == 1) && (reg == 7)) size = 2;
    return cpu->a[reg]-size;
  } else {
    if((size == 1) && (reg == 7)) size = 2; /* SP should get mad, not even */
    cpu->a[reg]-=size;
    return cpu->a[reg];
  }
}

static LONG ea_addr_101(struct cpu *cpu, int reg)
{
  int o;

  o = mmu_read_word(cpu->pc);
  if(o&0x8000) o |= 0xffff0000;

  if(!rmw) {
    cpu->pc += 2;
  }
  return cpu->a[reg]+o;
}

static LONG ea_addr_110(struct cpu *cpu, int reg)
{
  int d,o,r,a,rs;

  d = mmu_read_word(cpu->pc);
  if(!rmw) {
    cpu->pc += 2;
  }
  a = d&0x8000;
  r = (d&0x7000)>>12;
  rs = (d&0x800);
  d = d&0xff;
  if(d&0x80) d |= 0xffffff00;
  if(rs) {
    if(a)
      o = cpu->a[r];
    else
      o = cpu->d[r];
  } else {
    if(a)
      o = cpu->a[r]&0xffff;
    else
      o = cpu->d[r]&0xffff;
    if(o&0x8000) o |= 0xffff0000;
  }
  return cpu->a[reg]+o+d;
}

static LONG ea_addr_111_pcxn(struct cpu *cpu)
{
  int d,o,r,a,rs;

  d = mmu_read_word(cpu->pc);
  cpu->pc += 2;
  a = d&0x8000;
  r = (d&0x7000)>>12;
  rs = (d&0x800);
  d = d&0xff;
  if(d&0x80) d |= 0xffffff00;
  if(rs) {
    if(a)
      o = cpu->a[r];
    else
      o = cpu->d[r];
  } else {
    if(a)
      o = cpu->a[r]&0xffff;
    else
      o = cpu->d[r]&0xffff;
    if(o&0x8000) o |= 0xffff0000;
  }
  return cpu->pc+o+d-2;
}

static LONG ea_addr_111(struct cpu *cpu, int reg)
{
  LONG a;

  switch(reg) {
  case 0:
    ADD_CYCLE_EA(8);
    a = mmu_read_word(cpu->pc);
    if(a&0x8000) a |= 0xffff0000;
    if(!rmw) {
      cpu->pc += 2;
    }
    return a;
  case 1:
    ADD_CYCLE_EA(12);
    a = mmu_read_long(cpu->pc);
    if(!rmw) {
      cpu->pc += 4;
    }
    return a;
  case 2:
    ADD_CYCLE_EA(8);
    a = mmu_read_word(cpu->pc);
    if(a&0x8000) a |= 0xffff0000;
    cpu->pc += 2;
    return a+cpu->pc-2;
  case 3:
    ADD_CYCLE_EA(10);
    return ea_addr_111_pcxn(cpu);
  case 4:
    return 0;
  case 5:
    return 0;
  case 6:
    return 0;
  case 7:
    return 0;
  }
  return 0;
}

BYTE ea_read_byte(struct cpu *cpu, int mode, int noupdate)
{
  LONG addr;
  BYTE i;

  if(noupdate) rmw = 1;

  switch((mode&0x38)>>3) {
  case 0:
    rmw = 0;
    return ea_read_000_b(cpu, mode&0x7);
    break;
  case 1:
    rmw = 0;
    return ea_read_001_b(cpu, mode&0x7);
    break;
  case 2:
    ADD_CYCLE_EA(4);
    addr = ea_addr_010(cpu, mode&0x7);
    break;
  case 3:
    ADD_CYCLE_EA(4);
    addr = ea_addr_011(cpu, mode&0x7, 1);
    break;
  case 4:
    ADD_CYCLE_EA(6);
    addr = ea_addr_100(cpu, mode&0x7, 1);
    break;
  case 5:
    ADD_CYCLE_EA(8);
    addr = ea_addr_101(cpu, mode&0x7);
    break;
  case 6:
    ADD_CYCLE_EA(10);
    addr = ea_addr_110(cpu, mode&0x7);
    break;
  case 7:
    if((mode&0x7) == 4) {
      ADD_CYCLE_EA(4);
      i = mmu_read_word(cpu->pc)&0xff;
      cpu->pc += 2;
      rmw = 0;
      return i;
    } else {
      addr = ea_addr_111(cpu, mode&0x7);
    }
    break;
  default:
    addr = 0;
  }
  rmw = 0;
  return mmu_read_byte(addr);
}

WORD ea_read_word(struct cpu *cpu, int mode, int noupdate)
{
  LONG addr;
  WORD i;

  if(noupdate) rmw = 1;

  switch((mode&0x38)>>3) {
  case 0:
    rmw = 0;
    return ea_read_000_w(cpu, mode&0x7);
    break;
  case 1:
    rmw = 0;
    return ea_read_001_w(cpu, mode&0x7);
    break;
  case 2:
    ADD_CYCLE_EA(4);
    addr = ea_addr_010(cpu, mode&0x7);
    break;
  case 3:
    ADD_CYCLE_EA(4);
    addr = ea_addr_011(cpu, mode&0x7, 2);
    break;
  case 4:
    ADD_CYCLE_EA(6);
    addr = ea_addr_100(cpu, mode&0x7, 2);
    break;
  case 5:
    ADD_CYCLE_EA(8);
    addr = ea_addr_101(cpu, mode&0x7);
    break;
  case 6:
    ADD_CYCLE_EA(10);
    addr = ea_addr_110(cpu, mode&0x7);
    break;
  case 7:
    if((mode&0x7) == 4) {
      ADD_CYCLE_EA(4);
      i = mmu_read_word(cpu->pc);
      cpu->pc += 2;
      rmw = 0;
      return i;
    } else {
      addr = ea_addr_111(cpu, mode&0x7);
    }
    break;
  default:
    addr = 0;
  }
  rmw = 0;
  return mmu_read_word(addr);
}

LONG ea_read_long(struct cpu *cpu, int mode, int noupdate)
{
  LONG addr;
  LONG i;

  if(noupdate) rmw = 1;

  switch((mode&0x38)>>3) {
  case 0:
    rmw = 0;
    return ea_read_000_l(cpu, mode&0x7);
    break;
  case 1:
    rmw = 0;
    return ea_read_001_l(cpu, mode&0x7);
    break;
  case 2:
    ADD_CYCLE_EA(8);
    addr = ea_addr_010(cpu, mode&0x7);
    break;
  case 3:
    ADD_CYCLE_EA(8);
    addr = ea_addr_011(cpu, mode&0x7, 4);
    break;
  case 4:
    ADD_CYCLE_EA(10);
    addr = ea_addr_100(cpu, mode&0x7, 4);
    break;
  case 5:
    ADD_CYCLE_EA(12);
    addr = ea_addr_101(cpu, mode&0x7);
    break;
  case 6:
    ADD_CYCLE_EA(14);
    addr = ea_addr_110(cpu, mode&0x7);
    break;
  case 7:
    if((mode&0x7) == 4) {
      ADD_CYCLE_EA(8);
      i = mmu_read_long(cpu->pc);
      cpu->pc += 4;
      rmw = 0;
      return i;
    } else {
      ADD_CYCLE_EA(4);
      addr = ea_addr_111(cpu, mode&0x7);
    }
    break;
  default:
    addr = 0;
  }
  rmw = 0;
  return mmu_read_long(addr);
}

void ea_write_byte(struct cpu *cpu, int mode, BYTE data)
{
  LONG addr;

  rmw = 0;

  switch((mode&0x38)>>3) {
  case 0:
    ea_write_000_b(cpu, mode&0x7, data);
    return;
  case 1:
    return;
  case 2:
    ADD_CYCLE_EA(4);
    addr = ea_addr_010(cpu, mode&0x7);
    break;
  case 3:
    ADD_CYCLE_EA(4);
    addr = ea_addr_011(cpu, mode&0x7, 1);
    break;
  case 4:
    if(cpu->cyclecomp) {
      ADD_CYCLE_EA(4);
    } else {
      ADD_CYCLE_EA(6);
    }
    addr = ea_addr_100(cpu, mode&0x7, 1);
    break;
  case 5:
    ADD_CYCLE_EA(8);
    addr = ea_addr_101(cpu, mode&0x7);
    break;
  case 6:
    ADD_CYCLE_EA(10);
    addr = ea_addr_110(cpu, mode&0x7);
    break;
  case 7:
    addr = ea_addr_111(cpu, mode&0x7);
    break;
  default:
    addr = 0;
  }
  if(ea_prefetch_before_write) {
    cpu_prefetch();
    ea_clear_prefetch_before_write();
  }
  mmu_write_byte(addr, data);
}

void ea_write_word(struct cpu *cpu, int mode, WORD data)
{
  LONG addr;

  rmw = 0;

  switch((mode&0x38)>>3) {
  case 0:
    ea_write_000_w(cpu, mode&0x7, data);
    return;
  case 1:
    return;
  case 2:
    ADD_CYCLE_EA(4);
    addr = ea_addr_010(cpu, mode&0x7);
    break;
  case 3:
    ADD_CYCLE_EA(4);
    addr = ea_addr_011(cpu, mode&0x7, 2);
    break;
  case 4:
    if(cpu->cyclecomp) {
      ADD_CYCLE_EA(4);
    } else {
      ADD_CYCLE_EA(6);
    }
    addr = ea_addr_100(cpu, mode&0x7, 2);
    break;
  case 5:
    ADD_CYCLE_EA(8);
    addr = ea_addr_101(cpu, mode&0x7);
    break;
  case 6:
    ADD_CYCLE_EA(10);
    addr = ea_addr_110(cpu, mode&0x7);
    break;
  case 7:
    addr = ea_addr_111(cpu, mode&0x7);
    break;
  default:
    addr = 0;
  }
  if(ea_prefetch_before_write) {
    cpu_prefetch();
    ea_clear_prefetch_before_write();
  }
  mmu_write_word(addr, data);
}

void ea_write_long(struct cpu *cpu, int mode, LONG data)
{
  LONG addr;

  rmw = 0;

  switch((mode&0x38)>>3) {
  case 0:
    ea_write_000_l(cpu, mode&0x7, data);
    return;
  case 1:
    return;
  case 2:
    ADD_CYCLE_EA(8);
    addr = ea_addr_010(cpu, mode&0x7);
    break;
  case 3:
    ADD_CYCLE_EA(8);
    addr = ea_addr_011(cpu, mode&0x7, 4);
    break;
  case 4:
    if(cpu->cyclecomp) {
      ADD_CYCLE_EA(8);
    } else {
      ADD_CYCLE_EA(10);
    }
    addr = ea_addr_100(cpu, mode&0x7, 4);
    break;
  case 5:
    ADD_CYCLE_EA(12);
    addr = ea_addr_101(cpu, mode&0x7);
    break;
  case 6:
    ADD_CYCLE_EA(14);
    addr = ea_addr_110(cpu, mode&0x7);
    break;
  case 7:
    ADD_CYCLE_EA(4);
    addr = ea_addr_111(cpu, mode&0x7);
    break;
  default:
    addr = 0;
  }
  if(ea_prefetch_before_write) {
    cpu_prefetch();
    ea_clear_prefetch_before_write();
  }
  mmu_write_long(addr, data);
}

LONG ea_get_addr(struct cpu *cpu, int mode)
{
  ENTER;

  switch((mode&0x38)>>3) {
  case 0:
    return 0;
  case 1:
    return 0;
  case 2:
    ADD_CYCLE_EA(4);
    return ea_addr_010(cpu, mode&0x7);
  case 3: /* only for MOVEM */
    ADD_CYCLE_EA(4);
    return ea_addr_010(cpu, mode&0x7);
  case 4: /* only for MOVEM */
    ADD_CYCLE_EA(4);
    return ea_addr_010(cpu, mode&0x7);
  case 5:
    if(cpu->cyclecomp) {
      ADD_CYCLE_EA(6);
    } else {
      ADD_CYCLE_EA(8);
    }
    return ea_addr_101(cpu, mode&0x7);
  case 6:
    if(cpu->cyclecomp) {
      ADD_CYCLE_EA(10);
    } else {
      ADD_CYCLE_EA(12);
    }
    return ea_addr_110(cpu, mode&0x7);
  case 7:
    if(cpu->cyclecomp) {
      ADD_CYCLE_EA(-2);
    }
    if((mode&0x7) == 3) {
      ADD_CYCLE_EA(2);
    }
    return ea_addr_111(cpu, mode&0x7);
  }
  return 0;
}

static void ea_print_111(struct cprint *cprint, int mode, int size)
{
  LONG a;
  int d,r,l;
  char *str = cprint->data;
  LONG addr = cprint->addr + cprint->size;

  switch(mode&0x7) {
  case 0:
    a = mmu_read_word_print(addr);
    if(a&0x8000) a |= 0xffff0000;
    cprint_set_label(a, NULL);
    if(cprint_find_label(a)) {
      sprintf(str, "%s%s.W", str, cprint_find_label(a));
    } else {
      sprintf(str, "%s$%x.W", str, a);
    }
    cprint->size += 2;
    return;
  case 1:
    a = mmu_read_long_print(addr);
    cprint_set_label(a, NULL);
    if(cprint_find_label(a)) {
      sprintf(str, "%s%s", str, cprint_find_label(a));
    } else {
      sprintf(str, "%s$%x", str, a);
    }
    cprint->size += 4;
    return;
  case 2:
    a = mmu_read_word_print(addr);
    if(a&0x8000) a |= 0xffff0000;
    a += addr;
    cprint_set_label(a, NULL);
    if(cprint_find_label(a)) {
      sprintf(str, "%s%s(PC)", str, cprint_find_label(a));
    } else {
      sprintf(str, "%s$%x(PC)", str, a);
    }
    cprint->size += 2;
    return;
  case 3:
    a = mmu_read_word_print(addr);
    d = a&0x8000;
    r = (a&0x7000)>>12;
    l = a&0x800;
    if(a&0x80) a |= 0xffffff00;
    sprintf(str, "%s%d(PC, %c%d.%c)", str, a, d?'A':'D', r, l?'L':'W');
    cprint->size += 2;
    return;
  case 4:
    if(size == 0) {
      sprintf(str, "%s#$%x", str, mmu_read_word_print(addr)&0xff);
      cprint->size += 2;
      return;
    } else if(size == 1) {
      sprintf(str, "%s#$%x", str, mmu_read_word_print(addr));
      cprint->size += 2;
      return;
    } else {
      sprintf(str, "%s#$%x", str, mmu_read_long_print(addr));
      cprint->size += 4;
      return;
    }
  default:
    return;
  }
}

void ea_print(struct cprint *cprint, int mode, int size)
{
  int o;
  char *str = cprint->data;
  LONG addr = cprint->addr + cprint->size;

  switch((mode&0x38)>>3) {
  case 0:
    sprintf(str,"%sD%d", str, mode&0x7);
    return;
  case 1:
    sprintf(str, "%sA%d", str, mode&0x7);
    return;
  case 2:
    sprintf(str, "%s(A%d)", str, mode&0x7);
    return;
  case 3:
    sprintf(str, "%s(A%d)+", str, mode&0x7);
    return;
  case 4:
    sprintf(str, "%s-(A%d)", str, mode&0x7);
    return;
  case 5:
    o = mmu_read_word_print(addr);
    if(o&0x8000) o |= 0xffff0000;
    if(o > 127)
      sprintf(str, "%s$%x(A%d)", str, o, mode&0x7);
    else
      sprintf(str, "%s%d(A%d)", str, o, mode&0x7);
    cprint->size += 2;
    return;
  case 6:
    o = mmu_read_byte_print(addr+1)&0xff;
    if(o&0x80) o |= 0xffffff00;
    sprintf(str, "%s%d(A%d,%c%d.%c)",
	    str, 
	    o,
	    mode&0x7,
	    (mmu_read_byte_print(addr)&0x80)?'A':'D',
	    (mmu_read_byte_print(addr)>>4)&0x7,
	    (mmu_read_byte_print(addr)&0x8)?'L':'W');
    cprint->size += 2;
    return;
  case 7:
    ea_print_111(cprint, mode, size);
    return;
  default:
    return;
  }
}

int ea_valid(int mode, int mask)
{
  int m,r;

  m = (mode&0x38)>>3;
  r = mode&7;

  if((m == 1) && (mask&EA_INVALID_A)) {
    return 0;
  } else if((m == 0) && (mask&EA_INVALID_D)) {
    return 0;
  } else if((m == 3) && (mask&EA_INVALID_INC)) {
    return 0;
  } else if((m == 4) && (mask&EA_INVALID_DEC)) {
    return 0;
  } else if(m != 7) {
    return 1;
  } else if(m == 7) {
    if((r == 4) && (mask&EA_INVALID_I)) {
      return 0;
    } else if(((r == 2) || (r == 3)) && (mask&EA_INVALID_PC)) {
      return 0;
    } else if(r < 5) {
      return 1;
    }
  }

  return 0;
}

void ea_set_prefetch_before_write()
{
  ea_prefetch_before_write = 1;
}

void ea_clear_prefetch_before_write()
{
  ea_prefetch_before_write = 1;
}
