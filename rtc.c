#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"
#include "state.h"
#include "diag.h"

#define RTCSIZE 0x40
#define RTCBASE 0xfffc20

HANDLE_DIAGNOSTICS(rtc)

static BYTE rtc_read_byte(LONG addr)
{
  return 0;
}

static WORD rtc_read_word(LONG addr)
{
  return (rtc_read_byte(addr)<<8)|rtc_read_byte(addr+1);
}

static void rtc_write_byte(LONG addr, BYTE data)
{
}

static void rtc_write_word(LONG addr, WORD data)
{
  rtc_write_byte(addr, (data&0xff00)>>8);
  rtc_write_byte(addr+1, (data&0xff));
}

static int rtc_state_collect(struct mmu_state *state)
{
  state->size = 0;
  return STATE_VALID;
}

static void rtc_state_restore(struct mmu_state *state)
{
}

void rtc_init()
{
  struct mmu *rtc;

  rtc = mmu_create("RTC0", "Real-Time Clock");
  if(!rtc) {
    return;
  }
  rtc->start = RTCBASE;
  rtc->size = RTCSIZE;
  rtc->read_byte = rtc_read_byte;
  rtc->read_word = rtc_read_word;
  rtc->write_byte = rtc_write_byte;
  rtc->write_word = rtc_write_word;
  rtc->state_collect = rtc_state_collect;
  rtc->state_restore = rtc_state_restore;
  rtc->diagnostics = rtc_diagnostics;

  mmu_register(rtc);
}



