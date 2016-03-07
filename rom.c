#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "prefs.h"
#include "mmu.h"
#include "state.h"
#include "diag.h"

#define NEWTOS 0

#if NEWTOS
#define ROMSIZE 262144
#define ROMBASE 0xe00000
#else
#define ROMSIZE 196608
#define ROMBASE 0xfc0000
#endif

#define ROMSIZE2 8
#define ROMBASE2 0

static BYTE *memory;
static BYTE *memory2;

HANDLE_DIAGNOSTICS(rom)

static BYTE *real(LONG addr)
{

  if(addr < ROMBASE)
    return memory2+addr-ROMBASE2;
  else
    return memory+addr-ROMBASE;
}

static BYTE rom_read_byte(LONG addr)
{
  return *(real(addr));
}

static WORD rom_read_word(LONG addr)
{
  return (rom_read_byte(addr)<<8)|rom_read_byte(addr+1);
}

static LONG rom_read_long(LONG addr)
{
  return ((rom_read_byte(addr)<<24)|
       (rom_read_byte(addr+1)<<16)|
       (rom_read_byte(addr+2)<<8)|
       (rom_read_byte(addr+3)));
}

static int rom_state_collect(struct mmu_state *state)
{
  if(!strcmp("ROM0", state->id)) {
    state->size = ROMSIZE;
    state->data = (char *)malloc(state->size);
    if(state->data == NULL)
      return STATE_INVALID;
    memcpy(state->data, memory, state->size);
  } else {
    state->size = ROMSIZE2;
    state->data = (char *)malloc(state->size);
    if(state->data == NULL)
      return STATE_INVALID;
    memcpy(state->data, memory2, state->size);
  }
  return STATE_VALID;
}

static void rom_state_restore(struct mmu_state *state)
{
  long size;
  
  size = state->size;
  if(!strcmp("ROM0", state->id)) {
    if(state->size > ROMSIZE) size = ROMSIZE;
    memcpy(memory, state->data, size);
  } else {
    if(state->size > ROMSIZE2) size = ROMSIZE2;
    memcpy(memory2, state->data, size);
  }
}

void rom_init()
{
  struct mmu *rom,*rom2;
  FILE *f;

  memory = (BYTE *)malloc(sizeof(BYTE) * ROMSIZE);
  if(!memory) {
    return;
  }
  rom = (struct mmu *)malloc(sizeof(struct mmu));
  if(!rom) {
    free(memory);
    return;
  }
  
  rom->start = ROMBASE;
  rom->size = ROMSIZE;
  memcpy(rom->id, "ROM0", 4);
  rom->name = strdup("ROM");
  rom->read_byte = rom_read_byte;
  rom->read_word = rom_read_word;
  rom->read_long = rom_read_long;
  rom->write_byte = NULL;
  rom->write_word = NULL;
  rom->write_long = NULL;
  rom->state_collect = rom_state_collect;
  rom->state_restore = rom_state_restore;
  rom->diagnostics = rom_diagnostics;

  mmu_register(rom);

  f = fopen(prefs.tosimage, "rb");
  if(!f) {
    FATAL("Could not open TOS image file");
  }
  if(fread(memory, 1, ROMSIZE, f) != ROMSIZE) {
    FATAL("Error reading TOS image file");
  }
  fclose(f);
  
  memory2 = (BYTE *)malloc(sizeof(BYTE) * ROMSIZE2);
  if(!memory2) {
    return;
  }
  rom2 = (struct mmu *)malloc(sizeof(struct mmu));
  if(!rom2) {
    free(memory2);
    return;
  }

  memcpy(memory2, memory, ROMSIZE2);

  rom2->start = ROMBASE2; /* First 8 bytes of memory is ROM */
  rom2->size = ROMSIZE2;
  memcpy(rom2->id, "ROM1", 4);
  rom2->name = strdup("First 8 bytes of memory");
  rom2->read_byte = rom_read_byte;
  rom2->read_word = rom_read_word;
  rom2->read_long = rom_read_long;
  rom2->write_byte = NULL;
  rom2->write_word = NULL;
  rom2->write_long = NULL;
  rom2->state_collect = rom_state_collect;
  rom2->state_restore = rom_state_restore;
  rom2->diagnostics = rom_diagnostics;

  mmu_register(rom2);
}
