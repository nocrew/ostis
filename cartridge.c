#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"

#define CARTRIDGESIZE 131072
#define CARTRIDGEBASE 0xfa0000

static BYTE *memory;

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

void cartridge_init()
{
  struct mmu *cartridge;

  memory = (BYTE *)malloc(sizeof(BYTE) * CARTRIDGESIZE);
  if(!memory) {
    return;
  }
  cartridge = (struct mmu *)malloc(sizeof(struct mmu));
  if(!cartridge) {
    free(memory);
    return;
  }
  
  /* Cartridge signature: 0xfa52235f
   *
   * Only for testing purposes...
   */

#if 0
  memory[0] = 0xfa;
  memory[1] = 0x52;
  memory[2] = 0x23;
  memory[3] = 0x5f;
#endif

  cartridge->start = CARTRIDGEBASE;
  cartridge->size = CARTRIDGESIZE;
  cartridge->name = strdup("Cartridge");
  cartridge->read_byte = cartridge_read_byte;
  cartridge->read_word = cartridge_read_word;
  cartridge->read_long = cartridge_read_long;
  cartridge->write_byte = NULL;
  cartridge->write_word = NULL;
  cartridge->write_long = NULL;

  mmu_register(cartridge);
}







