#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"
#include "state.h"

static BYTE mmu_fallback_read_byte(LONG addr)
{
  return 0xff;
}

static WORD mmu_fallback_read_word(LONG addr)
{
  return 0xffff;
}

static LONG mmu_fallback_read_long(LONG addr)
{
  return 0xffffffff;
}

static void mmu_fallback_write_byte(LONG addr, BYTE data)
{
  /* Do nothing here */
}

static void mmu_fallback_write_word(LONG addr, WORD data)
{
  /* Do nothing here */
}

static void mmu_fallback_write_long(LONG addr, LONG data)
{
  /* Do nothing here */
}

static int mmu_fallback_state_collect(struct mmu_state *state)
{
  state->size = 0;
  return STATE_VALID;
}

static void mmu_fallback_state_restore(struct mmu_state *state)
{
}

static void mmu_fallback_register(char *name, LONG addr, int size)
{
  struct mmu *mmu_fallback;
  mmu_fallback = (struct mmu *)malloc(sizeof(struct mmu));
  if(!mmu_fallback) {
    return;
  }

  mmu_fallback->start = addr;
  mmu_fallback->size = size;
  memcpy(mmu_fallback->id, name, 4);
  mmu_fallback->name = strdup(name);
  mmu_fallback->read_byte = mmu_fallback_read_byte;
  mmu_fallback->read_word = mmu_fallback_read_word;
  mmu_fallback->read_long = mmu_fallback_read_long;
  mmu_fallback->write_byte = mmu_fallback_write_byte;
  mmu_fallback->write_word = mmu_fallback_write_word;
  mmu_fallback->write_long = mmu_fallback_write_long;
  mmu_fallback->state_collect = mmu_fallback_state_collect;
  mmu_fallback->state_restore = mmu_fallback_state_restore;

  mmu_register(mmu_fallback);
}

void mmu_fallback_init()
{
  /* Memory areas that will not actually do anything, but also not cause bus errors to occur */
  mmu_fallback_register("FBK0", 0xfffa30, 16);
  mmu_fallback_register("FBK1", 0xfffc00, 32);
  mmu_fallback_register("FBK2", 0xfffc20, 480);
}
