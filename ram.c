#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"
#include "state.h"

#define RAMSIZE (4*1048576)-8
#define RAMBASE 8
#define RAMCFGSIZE 1
#define RAMCFGBASE 0xff8001
#define RAM_PHYSMAX 0x100000-1

static BYTE ramcfg;

static BYTE *memory;
static BYTE *real(LONG addr)
{
  return memory+addr-RAMBASE;
}

static BYTE ram_read_byte(LONG addr)
{
  if(addr > RAM_PHYSMAX) return 0;
  return *(real(addr));
}

static WORD ram_read_word(LONG addr)
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

void ram_init()
{
  struct mmu *ram,*cfg;
#if 0
  int i;
#endif

  memory = (BYTE *)malloc(sizeof(BYTE) * RAMSIZE);
  if(!memory) {
    return;
  }
  ram = (struct mmu *)malloc(sizeof(struct mmu));
  if(!ram) {
    free(memory);
    return;
  }
  ram->start = RAMBASE;
  ram->size = RAMSIZE;
  memcpy(ram->id, "RAM0", 4);
  ram->name = strdup("RAM");
  ram->read_byte = ram_read_byte;
  ram->read_word = ram_read_word;
  ram->read_long = ram_read_long;
  ram->write_byte = ram_write_byte;
  ram->write_word = ram_write_word;
  ram->write_long = ram_write_long;
  ram->state_collect = ram_state_collect;
  ram->state_restore = ram_state_restore;

#if 0
  ram_write_long(0x420, 0x752019f3);
  ram_write_long(0x43a, 0x237698aa);
  ram_write_long(0x51a, 0x5555aaaa);
  ram_write_byte(0x424, 10);
  ram_write_byte(0x436, RAMSIZE+RAMBASE-0x8000);
  ram_write_long(0x42e, RAMSIZE+RAMBASE);
#endif

#if 0
  for(i=0;i<RAMSIZE;i+=4) {
    memory[i] = 0xde;
    memory[i+1] = 0xad;
    memory[i+2] = 0xbe;
    memory[i+3] = 0xef;
  }
#endif

  mmu_register(ram);

  cfg = (struct mmu *)malloc(sizeof(struct mmu));
  if(!cfg) {
    return;
  }
  cfg->start = RAMCFGBASE;
  cfg->size = RAMCFGSIZE;
  cfg->name = strdup("RAM Configuration");
  cfg->read_byte = ramcfg_read_byte;
  cfg->read_word = NULL;
  cfg->read_long = NULL;
  cfg->write_byte = ramcfg_write_byte;
  cfg->write_word = NULL;
  cfg->write_long = NULL;

  mmu_register(cfg);
}
