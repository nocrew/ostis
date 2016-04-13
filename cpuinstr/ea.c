#include "cpu.h"
#include "ea.h"

static int ea_cycles;
static LONG ea_address;
static LONG ea_data;
static LONG ea_mask;
static WORD (*ea_read)(LONG);

typedef void (*uop_t)(void);
static uop_t *ea_upc;

static LONG sign_ext_8(BYTE x)
{
  if(x & 0x80)
    x |= 0xffffff00;
  else
    x &= 0xff;
  return x;
}

static LONG sign_ext_16(BYTE x)
{
  if(x & 0x8000)
    x |= 0xffff0000;
  else
    x &= 0xffff;
  return x;
}

static WORD ea_byte(LONG address)
{
  return bus_read_byte(address);
}

static WORD fetch(void)
{
  LONG addr = cpu->pc;
  cpu->pc += 2;
  return bus_read_word(addr);
}

// Do nothing.
static void n(void)
{
}

// Read from memory.
static void r(void)
{
  ea_data = (ea_data << 16) + ea_read(ea_address);
  ea_address += 2;
}

// Fetch from program.
static void p(void)
{
  ea_data = (ea_data << 16) + fetch();
}

static void a(void)
{
  ea_address = ea_data;
}

static void idx(void)
{
  int reg = (ea_data >> 12) & 7;
  int offset;

  ea_address += sign_ext_8(ea_data);

  if(ea_data & 0x8000)
    offset = cpu->a[reg];
  else
    offset = cpu->d[reg];
  if((ea_data & 0x800) == 0)
    offset = sign_ext_16(offset);
    
  ea_address += offset;
}

static uop_t indirect_uops[] = { n, r, n };
static uop_t predecrement_uops[] = { r, n, r, n };
static uop_t displacement_uops[] = { n, r, n, r, n };
static uop_t index_uops[] = { p, idx, r, n, r, n };
static uop_t absolute_uops[] = { n, p, a, r, n, r, n };
static uop_t immediate_uops[] = { n, p, n };

void ea_begin_read(struct cpu *cpu, WORD op)
{
  int reg = op & 7;
  int mode = op & 0x3f;
  int long_cycles = 0;
  int size = 0;

  ea_data = 0;
  ea_mask = ~0;
  ea_read = bus_read_word;

  switch(op & 0xc0) {
  case 0:
    size = (reg == 7 ? 2 : 1); // SP should get even, not mad
    ea_mask = 0xff;
    ea_read = ea_byte;
    break;
  case 1:
    size = 2;
    ea_mask = 0xffff;
    break;
  case 2:
    size = 4;
    long_cycles = 4;
    break;
  }

  if(mode < 0x38)
    mode &= 0x38;

  switch(mode) {
  case 0x00:
    ea_cycles = 0;
    ea_data = cpu->d[reg];
    break;
  case 0x08:
    ea_cycles = 0;
    ea_data = cpu->a[reg];
    break;
  case 0x10:
    ea_cycles = 4 + long_cycles; // nr(nr)
    ea_data = ea_read(cpu->a[reg]);
    ea_address = cpu->a[reg] + 2;
    ea_upc = indirect_uops;
    break;
  case 0x18:
    ea_cycles = 4 + long_cycles; // nr(nr)
    ea_data = ea_read(cpu->a[reg]);
    ea_address = cpu->a[reg] + 2;
    cpu->a[reg] += size;
    ea_upc = indirect_uops;
    break;
  case 0x20:
    ea_cycles = 6 + long_cycles; // nnr(nr)
    cpu->a[reg] -= size;
    ea_address = cpu->a[reg];
    ea_upc = predecrement_uops;
    break;
  case 0x28:
    ea_cycles = 8 + long_cycles; // npnr(nr)
    ea_address = cpu->a[reg] + sign_ext_16(fetch());
    ea_upc = displacement_uops;
    break;
  case 0x30:
    ea_cycles = 10 + long_cycles; // nnpnr(nr)
    ea_address = cpu->a[reg];
    ea_upc = index_uops;
    break;
  case 0x38:
    ea_cycles = 8 + long_cycles; // npnr(nr)
    ea_data = sign_ext_16(fetch());
    ea_upc = absolute_uops + 2;
    break;
  case 0x39:
    ea_cycles = 12 + long_cycles; // npnpnr(nr)
    ea_data = fetch();
    ea_upc = absolute_uops;
    break;
  case 0x3A:
    ea_cycles = 8 + long_cycles; // npnr(nr)
    ea_address = cpu->pc;
    ea_address += sign_ext_16(fetch());
    ea_upc = displacement_uops;
    break;
  case 0x3B:
    ea_cycles = 10 + long_cycles; // nnpnr(nr)
    ea_address = cpu->pc;
    ea_upc = index_uops;
    break;
  case 0x3C:
    ea_cycles = 4 + long_cycles; // np(np)
    ea_data = fetch();
    ea_upc = immediate_uops;
    break;
  }
}

int ea_step(LONG *operand)
{
  if(ea_cycles == 0) {
    *operand = ea_data & ea_mask;
    return 1;
  }

  ea_cycles--;
  if((ea_cycles & 1) == 0) {
    (*ea_upc)();
    ea_upc++;
  }
  
  return 0;
}
