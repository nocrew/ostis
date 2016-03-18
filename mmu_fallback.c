#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"
#include "state.h"
#include "diag.h"

HANDLE_DIAGNOSTICS(mmu_fallback)

static BYTE mmu_fallback_read_byte(LONG addr)
{
  return 0xff;
}

static WORD mmu_fallback_read_word(LONG addr)
{
  return 0xffff;
}

static void mmu_fallback_write_byte(LONG addr, BYTE data)
{
  /* Do nothing here */
}

static void mmu_fallback_write_word(LONG addr, WORD data)
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
  mmu_fallback = mmu_create(name, "Fallback");

  mmu_fallback->start = addr;
  mmu_fallback->size = size;
  mmu_fallback->read_byte = mmu_fallback_read_byte;
  mmu_fallback->read_word = mmu_fallback_read_word;
  mmu_fallback->write_byte = mmu_fallback_write_byte;
  mmu_fallback->write_word = mmu_fallback_write_word;
  mmu_fallback->state_collect = mmu_fallback_state_collect;
  mmu_fallback->state_restore = mmu_fallback_state_restore;
  mmu_fallback->diagnostics = mmu_fallback_diagnostics;

  mmu_register(mmu_fallback);
}

void mmu_fallback_init()
{
  /* Memory areas that will not actually do anything, but also not cause bus errors to occur */
  mmu_fallback_register("FBK0", 0xfffa30, 16);
  mmu_fallback_register("FBK1", 0xfffc00, 32);
  mmu_fallback_register("FBK2", 0xfffc20, 480);
}
