/*
 * EA calculation and operand access state machine.
 *
 * The primary entry points are:
 * - ea_begin_read - start a read operation.
 * - ea_begin_modify - start a write operation to the same location as
 *   the previous read operation.
 * - ea_begin_write - start a new write operation.
 *
 * After this, the ea_done function should be called to advance the
 * state machine.  It returns 1 when the calculation has finished.
 * May return immediately for i.e. register accesses which takes 0
 * cycles.
 */

#include "cpu.h"
#include "ea.h"
#include "ucode.h"

static LONG ea_address;
static LONG ea_data;
static LONG ea_mask;
static WORD (*ea_read)(LONG);
static void (*ea_write)(LONG, WORD);

static LONG sign_ext_8(LONG x)
{
  if(x & 0x80)
    x |= 0xffffff00;
  else
    x &= 0xff;
  return x;
}

static LONG sign_ext_16(LONG x)
{
  if(x & 0x8000)
    x |= 0xffff0000;
  else
    x &= 0xffff;
  return x;
}

static WORD read_byte(LONG address)
{
  return bus_read_byte(address);
}

static void write_byte(LONG address, WORD data)
{
  bus_write_byte(address, data);
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

// Write to memory.
static void w(void)
{
  ea_address -= 2;
  ea_write(ea_address, ea_data);
  ea_data >>= 16;
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

static void sa(void)
{
  ea_address += sign_ext_16(ea_data);
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

static uop_t indirect_uops[] = { r, n, r, n };
static uop_t predecrement_uops[] = { n, r, n, r, n };
static uop_t displacement_uops[] = { p, sa, r, n, r, n };
static uop_t index_uops[] = { n, p, idx, r, n, r, n };
static uop_t absolute_short_uops[] = { p, sa, r, n, r, n };
static uop_t absolute_long_uops[] = { p, n, p, a, r, n, r, n };
static uop_t immediate_uops[] = { p, n, p, n };

void ea_begin_read(struct cpu *cpu, WORD op)
{
  int reg = op & 7;
  int mode = op & 0x3f;
  int long_cycles = 0;
  int size = 0;

  ea_data = 0;
  ea_mask = ~0;
  ea_read = bus_read_word;

  switch((op >> 6) & 3) {
  case 0:
    size = (reg == 7 ? 2 : 1); // SP should get even, not mad
    ea_mask = 0xff;
    ea_read = read_byte;
    break;
  case 1:
    size = 2;
    ea_mask = 0xffff;
    break;
  case 2:
    size = 4;
    long_cycles = 2;
    break;
  }

  if(mode < 0x38)
    mode &= 0x38;

  switch(mode) {
  case 0x00:
    ea_data = cpu->d[reg];
    ujump(0, 0);
    break;
  case 0x08:
    ea_data = cpu->a[reg];
    ujump(0, 0);
    break;
  case 0x10:
    ea_address = cpu->a[reg];
    ujump(indirect_uops, 2 + long_cycles); // nr(nr)
    break;
  case 0x18:
    ea_address = cpu->a[reg];
    cpu->a[reg] += size;
    ujump(indirect_uops, 2 + long_cycles); // nr(nr);
    break;
  case 0x20:
    cpu->a[reg] -= size;
    ea_address = cpu->a[reg];
    ujump(predecrement_uops, 3 + long_cycles); // nnr(nr);
    break;
  case 0x28:
    ea_address = cpu->a[reg];
    ujump(displacement_uops, 4 + long_cycles); // npnr(nr);
    break;
  case 0x30:
    ea_address = cpu->a[reg];
    ujump(index_uops, 5 + long_cycles); // nnpnr(nr);
    break;
  case 0x38:
    ea_address = 0;
    ujump(absolute_short_uops, 4 + long_cycles); // npnr(nr)
    break;
  case 0x39:
    ujump(absolute_long_uops, 6 + long_cycles); // npnpnr(nr);
    break;
  case 0x3A:
    ea_address = cpu->pc;
    ujump(displacement_uops, 4 + long_cycles); // npnr(nr)
    break;
  case 0x3B:
    ea_address = cpu->pc;
    ujump(index_uops, 5 + long_cycles); // nnpnr(nr)
    break;
  case 0x3C:
    ujump(immediate_uops, 2 + long_cycles); // np(np)
    break;
  }
}

static uop_t write_reg_uops[] = { n, n, n };
static uop_t write_mem_uops[] = { w, n, w, n };

// dw, dl, aw, al are the number of additional microcycles needed to
// write a data or address register with a word or a long.
void ea_begin_modify(struct cpu *cpu, WORD op, LONG data,
		     int dw, int dl, int aw, int al)
{
  //						Byte/Word	Long
  //						Dn, An, Mem	Dn, An, Mem
  // EORI, ORI, ANDI, SUBI, ADDI		0   -   nw	nn  -   nwnW
  // BCHG, BSET					-   -   nw	n/nn-   -
  // BCLR					-   -   nw	nn/nnn- -
  // CLR, NEGX, NEG, NOT			0   -   nw	n   -   nwnW
  // NBCD					n   -   nw	-   -   -
  // ADDQ, SUBQ					0   nn  nw	nn  nn  nwnw
  // Scc					0/n -   nw	-   -   -
  // AND, OR, ADD, SUB				0   -   nw	n/nn-   nwnW
  // ADDA, SUBA					-   nn  -	-   n/nn-
  // EOR					0   -   nw	nn  -   nwnW
  // shift					n   -	nw	nn  -   -

  int mode = op & 0x3f;
  int long_cycles = 0;

  ea_data = 0;
  ea_write = bus_write_word;

  switch((op >> 6) & 3) {
  case 0:
    ea_write = write_byte;
    break;
  case 2:
    long_cycles = 2;
    break;
  }

  if(mode < 0x38)
    mode &= 0x38;

  switch(mode) {
  case 0x00:
    switch((op >> 6) & 3) {
    case 0:
      cpu->d[op & 7] = (cpu->d[op & 7] & 0xffffff00) + (data & 0xff);
      break;
    case 1:
      cpu->d[op & 7] = (cpu->d[op & 7] & 0xffff0000) + (data & 0xffff);
      break;
    case 2:
      cpu->d[op & 7] = data;
      break;
    }
    ujump(write_reg_uops, long_cycles ? dl : dw);
    break;
  case 0x08:
    switch((op >> 6) & 3) {
    case 1:
      cpu->a[op & 7] = sign_ext_16(data);
      break;
    case 2:
      cpu->a[op & 7] = data;
      break;
    }
    ujump(write_reg_uops, long_cycles ? al : aw);
    break;
  case 0x10:
  case 0x18:
  case 0x20:
  case 0x28:
  case 0x30:
  case 0x38:
  case 0x39:
  case 0x3A:
  case 0x3B:
    ea_data = data;
    ujump(write_mem_uops, 2 + long_cycles);
    break;
  }
}

int ea_done(LONG *operand)
{
  if(ustep()) {
    *operand = ea_data & ea_mask;
    return 1;
  }

  return 0;
}
