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

static LONG rtc_read_long(LONG addr)
{
  return ((rtc_read_byte(addr)<<24)|
	  (rtc_read_byte(addr+1)<<16)|
	  (rtc_read_byte(addr+2)<<8)|
	  (rtc_read_byte(addr+3)));
}

static void rtc_write_byte(LONG addr, BYTE data)
{
}

static void rtc_write_word(LONG addr, WORD data)
{
  rtc_write_byte(addr, (data&0xff00)>>8);
  rtc_write_byte(addr+1, (data&0xff));
}

static void rtc_write_long(LONG addr, LONG data)
{
  rtc_write_byte(addr, (data&0xff000000)>>24);
  rtc_write_byte(addr+1, (data&0xff0000)>>16);
  rtc_write_byte(addr+2, (data&0xff00)>>8);
  rtc_write_byte(addr+3, (data&0xff));
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

  rtc = (struct mmu *)malloc(sizeof(struct mmu));
  if(!rtc) {
    return;
  }
  rtc->start = RTCBASE;
  rtc->size = RTCSIZE;
  memcpy(rtc->id, "RTC0", 4);
  rtc->name = strdup("RTC");
  rtc->read_byte = rtc_read_byte;
  rtc->read_word = rtc_read_word;
  rtc->read_long = rtc_read_long;
  rtc->write_byte = rtc_write_byte;
  rtc->write_word = rtc_write_word;
  rtc->write_long = rtc_write_long;
  rtc->state_collect = rtc_state_collect;
  rtc->state_restore = rtc_state_restore;
  rtc->diagnostics = rtc_diagnostics;

  mmu_register(rtc);
}



