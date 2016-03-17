#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"
#include "state.h"
#include "diag.h"

#define CARTRIDGESIZE 131072
#define CARTRIDGEBASE 0xfa0000

static BYTE *memory;

HANDLE_DIAGNOSTICS(cartridge)

static BYTE *real(LONG addr)
{
  return memory+addr-CARTRIDGEBASE;
}

static BYTE cartridge_read_byte(LONG addr)
{
  return *(real(addr));
}

static WORD cartridge_read_word(LONG addr)
{
  return (cartridge_read_byte(addr)<<8)|cartridge_read_byte(addr+1);
}

static LONG cartridge_read_long(LONG addr)
{
  return ((cartridge_read_byte(addr)<<24)|
       (cartridge_read_byte(addr+1)<<16)|
       (cartridge_read_byte(addr+2)<<8)|
       (cartridge_read_byte(addr+3)));
}

static int cartridge_state_collect(struct mmu_state *state)
{
  state->size = 0;
  return STATE_VALID;
}

static void cartridge_state_restore(struct mmu_state *state)
{
}

void cartridge_init(char *filename)
{
  struct mmu *cartridge;
  FILE *fp;
  int file_size = 0;

  memory = (BYTE *)xmalloc(sizeof(BYTE) * CARTRIDGESIZE);
  if(!memory) {
    return;
  }
  cartridge = mmu_create("CART", "Cartridge");

  cartridge->start = CARTRIDGEBASE;
  cartridge->size = CARTRIDGESIZE;
  cartridge->read_byte = cartridge_read_byte;
  cartridge->read_word = cartridge_read_word;
  cartridge->read_long = cartridge_read_long;
  cartridge->state_collect = cartridge_state_collect;
  cartridge->state_restore = cartridge_state_restore;
  cartridge->diagnostics = cartridge_diagnostics;

  mmu_register(cartridge);

  fp = fopen(filename, "rb");
  if(fp) {
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    if(file_size <= CARTRIDGESIZE) {
      if(fread(memory, 1, file_size, fp) != file_size) {
        ERROR("Unable to load %d bytes from %s", file_size, filename);
        /* Making sure cartridge memory area doesn't start with boot sequence */
        memory[0] = '\0';
      }
    } else {
      ERROR("Cannot load file larger than cartridge memory area");
    }
    fclose(fp);
  }
}
