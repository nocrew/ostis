#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"
#include "state.h"
#include "diag.h"

#define RAMSIZE (4*1048576)-8
#define RAMBASE 8
#define RAMCFGSIZE 1
#define RAMCFGBASE 0xff8001
#define RAM_PHYSMAX 0x100000-1

static BYTE ramcfg;
static BYTE *memory;

HANDLE_DIAGNOSTICS(ram)

static BYTE *real(LONG addr)
{
  return memory+addr-RAMBASE;
}

static BYTE ram_read_byte(LONG addr)
{
  if(addr > RAM_PHYSMAX) return 0;
  return *(real(addr));
}

WORD ram_read_word(LONG addr)
{
  return (ram_read_byte(addr)<<8)|ram_read_byte(addr+1);
}

static LONG ram_read_long(LONG addr)
{
  return ((ram_read_byte(addr)<<24)|
       (ram_read_byte(addr+1)<<16)|
       (ram_read_byte(addr+2)<<8)|
       (ram_read_byte(addr+3)));
}

static void ram_write_byte(LONG addr, BYTE data)
{
  *(real(addr)) = data;
}

static void ram_write_word(LONG addr, WORD data)
{
  ram_write_byte(addr, (data&0xff00)>>8);
  ram_write_byte(addr+1, (data&0xff));
}

static void ram_write_long(LONG addr, LONG data)
{
  ram_write_byte(addr, (data&0xff000000)>>24);
  ram_write_byte(addr+1, (data&0xff0000)>>16);
  ram_write_byte(addr+2, (data&0xff00)>>8);
  ram_write_byte(addr+3, (data&0xff));
}

static int ram_state_collect(struct mmu_state *state)
{
  state->size = RAM_PHYSMAX+1-RAMBASE;
  state->data = (char *)malloc(state->size);
  if(state->data == NULL)
    return STATE_INVALID;
  memcpy(state->data, memory, state->size);

  return STATE_VALID;
}

static void ram_state_restore(struct mmu_state *state)
{
  long size;

  size = state->size;
  if(state->size > (RAM_PHYSMAX+1-RAMBASE))
    size = RAM_PHYSMAX+1-RAMBASE;
  memcpy(memory, state->data, size);
}

void ram_clear(void)
{
  memset( memory, 0, sizeof(BYTE) * RAMSIZE );
}

static BYTE ramcfg_read_byte(LONG addr)
{
  return ramcfg;
}

static void ramcfg_write_byte(LONG addr, BYTE data)
{
  ramcfg = data;
}

static void cfg_diagnostics()
{
}

void ram_init()
{
  struct mmu *ram,*cfg;

  memory = (BYTE *)malloc(sizeof(BYTE) * RAMSIZE);
  if(!memory) {
    return;
  }
  ram = mmu_create("RAM0", "RAM");
  ram->start = RAMBASE;
  ram->size = RAMSIZE;
  ram->read_byte = ram_read_byte;
  ram->read_word = ram_read_word;
  ram->read_long = ram_read_long;
  ram->write_byte = ram_write_byte;
  ram->write_word = ram_write_word;
  ram->write_long = ram_write_long;
  ram->state_collect = ram_state_collect;
  ram->state_restore = ram_state_restore;
  ram->diagnostics = ram_diagnostics;

  mmu_register(ram);

  cfg = mmu_create("CFG0", "RAM Configuration");
  cfg->start = RAMCFGBASE;
  cfg->size = RAMCFGSIZE;
  cfg->read_byte = ramcfg_read_byte;
  cfg->write_byte = ramcfg_write_byte;
  cfg->diagnostics = cfg_diagnostics;

  mmu_register(cfg);
}
