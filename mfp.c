/*
 * MFP 68901	Connected to
 *
 * [CONTROL]
 *  CLK		4MHz
 *  RESET	RESET
 *  VCC		+5V
 *  GND		GND
 * [BUS]
 *  D0-D7	BUS:D0-D7
 *  RS1-RS5	BUS A1-A5
 *  CS		GLUE:MFPCS
 *  R/W		R/W
 *  DS		LDS
 *  DTACK	DTACK
 * [INTERRUPT CONTROL]
 *  IRQ		GLUE:MFPINT
 *  IEO		-
 *  IEI		GND
 *  IACK	IACK
 * [TIMERS]
 *  XTAL1	2.4576Hz
 *  XTAL2	2.4576Hz
 *  TAO		-
 *  TAI		PARALLEL:BUSY
 *  TBO		-
 *  TBI		GLUE:DE
 *  TCO		-
 *  TDO		MFP:TC, MFP:RC
 * [UART]
 *  RR		-
 *  SI		RS232:RD
 *  RC		MFP:TD0
 *  SO		RS232:SD
 *  TC		MFP:TD0
 *  TR		-
 * [GPIP]
 *  I0		PARALLEL:BUSY
 *  I1		RS232:CD
 *  I2		RS232:CS
 *  I3		-
 *  I4		ACIA:IRQ
 *  I5		FDC:INTR, HDC:INT
 *  I6		RS232:CI
 *  I7		MONITOR:MONO
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"
#include "mfp.h"
#include "state.h"
#include "diag.h"

#define MFPSIZE 48
#define MFPBASE 0xfffa00

#define GPIP   0
#define AER    1
#define DDR    2
#define IERA   3
#define IERB   4
#define IPRA   5
#define IPRB   6
#define ISRA   7
#define ISRB   8
#define IMRA   9
#define IMRB  10
#define VR    11
#define TACR  12
#define TBCR  13
#define TCDCR 14
#define TADR  15
#define TBDR  16
#define TCDR  17
#define TDDR  18
#define SCR   19
#define UCR   20
#define RSR   21
#define TSR   22
#define UDR   23

#define INT_TIMERA 13
#define INT_TIMERB  8
#define INT_TIMERC  5
#define INT_TIMERD  4

#define IER ((mfpreg[IERA]<<8)|mfpreg[IERB])
#define IPR ((mfpreg[IPRA]<<8)|mfpreg[IPRB])
#define ISR ((mfpreg[ISRA]<<8)|mfpreg[ISRB])
#define IMR ((mfpreg[IMRA]<<8)|mfpreg[IMRB])

#define MFPBASEFREQ 2457600
#define MFPFREQ (MFPBASEFREQ*313)

static long lastcpucnt;
static long precnt[4];
static int intnum[4] = { INT_TIMERA, INT_TIMERB, INT_TIMERC, INT_TIMERD };
static long divider[8] = { 0, 4, 10, 16, 50, 64, 100, 200 };
static BYTE mfpreg[24];
static BYTE timercnt[4];

static void mfp_do_interrupts(struct cpu *cpu);

HANDLE_DIAGNOSTICS(mfp)

static BYTE mfp_read_byte(LONG addr)
{
  int r;

  if(!(addr&1)) return 0;
  addr -= MFPBASE;
  r = (addr>>1)%24;

  switch(r) {
  case GPIP:
    return mfpreg[GPIP] | (monitor_sm124 ? 0 : 0x80);
  case TADR:
  case TBDR:
  case TCDR:
  case TDDR:
    return timercnt[r-TADR];
  default:
    return mfpreg[r];
  }
}

static WORD mfp_read_word(LONG addr)
{
  return (mfp_read_byte(addr)<<8)|mfp_read_byte(addr+1);
}

static void mfp_write_byte(LONG addr, BYTE data)
{
  int r;
  
  if(!(addr&1)) return;

  addr -= MFPBASE;
  r = (addr>>1)%24;

  switch(r) {
  case TACR:
    if(!(mfpreg[TACR]&0xf) && data&0x7) {
      if(data > 7) DEBUG("Not fully implemented");
      precnt[0] = 313*divider[data&0x7];
    }
    break;
  case TBCR:
    if(!(mfpreg[TBCR]&0xf) && data&0x7) {
      if(data > 7) DEBUG("Not fully implemented");
      precnt[1] = 313*divider[data&0x7];
    }
    break;
  case TCDCR:
    if(!(mfpreg[TCDCR]&0x70) && data&0x70) {
      precnt[2] = 313*divider[(data&0x70)>>4];
    }
    if(!(mfpreg[TCDCR]&0x7) && data&0x7) {
      precnt[3] = 313*divider[data&0x7];
    }
    break;
  case TADR:
    if(!(mfpreg[TACR]&0xf)) timercnt[0] = data;
    break;
  case TBDR:
    if(!(mfpreg[TBCR]&0xf)) timercnt[1] = data;
    break;
  case TCDR:
    if(!(mfpreg[TCDCR]&0x70)) timercnt[2] = data;
    break;
  case TDDR:
    if(!(mfpreg[TCDCR]&0x7)) timercnt[3] = data;
    break;
  }
  if((r != ISRA) && (r != ISRB)) {
    mfpreg[r] = data;
  } else {
    TRACE("Clear ISR: %02x %02x", mfpreg[r], data);
    mfpreg[r] &= data;
  }
}

static void mfp_write_word(LONG addr, WORD data)
{
  mfp_write_byte(addr, (data&0xff00)>>8);
  mfp_write_byte(addr+1, (data&0xff));
}

static int mfp_state_collect(struct mmu_state *state)
{
  int r;

  /* Size:
   * 
   * mfpreg[24]  == 24
   * precnt[4]   == 4*4
   * lastcpucnt  == 4
   * timercnt[4] == 4
   * ------------------
   * sum:           48
   */

  state->size = 48;
  state->data = (char *)malloc(state->size);
  if(state->data == NULL) {
    return STATE_INVALID;
  }
  for(r=0;r<24;r++) {
    state_write_mem_byte(&state->data[r], mfpreg[r]);
  }
  for(r=0;r<4;r++) {
    state_write_mem_long(&state->data[24+r*4], precnt[r]);
  }
  state_write_mem_long(&state->data[24+4*4], lastcpucnt);
  for(r=0;r<4;r++) {
    state_write_mem_byte(&state->data[24+4*4+4+r], timercnt[r]);
  }
  return STATE_VALID;
}

static void mfp_state_restore(struct mmu_state *state)
{
  int r;
  
  for(r=0;r<24;r++) {
    mfpreg[r] = state_read_mem_byte(&state->data[r]);
  }
  for(r=0;r<4;r++) {
    precnt[r] = state_read_mem_long(&state->data[24+r*4]);
  }
  lastcpucnt = state_read_mem_long(&state->data[24+4*4]);
  for(r=0;r<4;r++) {
    timercnt[r] = state_read_mem_byte(&state->data[24+4*4+4+r]);
  }
}

void mfp_init()
{
  struct mmu *mfp;

  mfp = mmu_create("MFP0", "MFP 68901");

  mfp->start = MFPBASE;
  mfp->size = MFPSIZE;
  mfp->read_byte = mfp_read_byte;
  mfp->read_word = mfp_read_word;
  mfp->write_byte = mfp_write_byte;
  mfp->write_word = mfp_write_word;
  mfp->state_collect = mfp_state_collect;
  mfp->state_restore = mfp_state_restore;
  mfp->diagnostics = mfp_diagnostics;
  mfp->interrupt = mfp_do_interrupts;

  mmu_register(mfp);
}

void mfp_print_status()
{
  int i;
  char *regnames[] = {
    "GPIP",    "AER",    "DDR",    "IERA",
    "IERB",    "IPRA",    "IPRB",    "ISRA",
    "ISRB",    "IMRA",    "IMRB",    "VR",
    "TACR",    "TBCR",    "TCDCR",    "TADR",
    "TBDR",    "TCDR",    "TDDR",    "SCR",
    "UCR",    "RSR",    "TSR",    "UDR"
  };

  printf("MFP: \n");
  for(i=0;i<24;i++) {
    printf("%5s: %02x ",regnames[i], mfpreg[i]);
    if(i%4 == 3) {
      printf("\n");
    }
  }
  
  printf("\n");
  for(i=0;i<4;i++)
    printf(" Prediv %c: %ld %d\n", i+'A', precnt[i], timercnt[i]);
  printf("\n");
}

static void update_timer(int tnum, long cycles)
{
  int d;
  int iermask;
  int ierreg;

  d = 0;

  switch(tnum) {
  case 0:
    if(!(mfpreg[TACR]&0xf)) return;
    if((mfpreg[TACR]&0xf) == 8) return;
    d = divider[(mfpreg[TACR]&0x7)];
    ierreg = 0;
    iermask = 0x20;
    break;
  case 1:
    if(!(mfpreg[TBCR]&0xf)) return;
    if((mfpreg[TBCR]&0xf) == 8) return;
    d = divider[(mfpreg[TBCR]&0x7)];
    ierreg = 0;
    iermask = 0x1;
    break;
  case 2:
    if(!(mfpreg[TCDCR]&0x70)) return;
    d = divider[(mfpreg[TCDCR]&0x70)>>4];
    ierreg = 1;
    iermask = 0x20;
    break;
  case 3:
    if(!(mfpreg[TCDCR]&0x7)) return;
    d = divider[(mfpreg[TCDCR]&0x7)];
    ierreg = 1;
    iermask = 0x10;
    break;
  }

  if(!d) return;

  precnt[tnum] -= cycles;
  if(precnt[tnum] < 0) {
    precnt[tnum] += d*313;
    timercnt[tnum]--;
    if(!timercnt[tnum]) {
      timercnt[tnum] = mfpreg[TADR+tnum];
      if(mfpreg[IERA+ierreg]&iermask) {
	mfp_set_pending(intnum[tnum]);
      }
    }
  }
}

static int mfp_get_GPIP(int bnum)
{
  return mfpreg[GPIP] & (1<<bnum);
}

static int mfp_get_AER(int bnum)
{
  return mfpreg[AER] & (1<<bnum);
}

void mfp_set_GPIP(int bnum)
{
  /* AER set to interrupt on rising edge */
  if(!mfp_get_GPIP(bnum) && mfp_get_AER(bnum)) {
    switch(bnum) {
    case MFP_GPIP_ACIA: /* IKBD/MIDI */
      mfp_set_pending(6);
      break;
    case MFP_GPIP_FDC: /* FDC */
      mfp_set_pending(7);
      break;
    }
  }
  mfpreg[GPIP] |= (1<<bnum);
}

void mfp_clr_GPIP(int bnum)
{
  /* AER set to interrupt on falling edge */
  if(mfp_get_GPIP(bnum) && !mfp_get_AER(bnum)) {
    switch(bnum) {
    case MFP_GPIP_ACIA: /* IKBD/MIDI */
      mfp_set_pending(6);
      break;
    case MFP_GPIP_FDC: /* FDC */
      mfp_set_pending(7);
      break;
    }
  }
  mfpreg[GPIP] &= ~(1<<bnum);
}

static void mfp_set_IPR(int inum)
{
  int t;

  t = IPR|(1<<inum);
  mfpreg[IPRA] = (t>>8);
  mfpreg[IPRB] = (t&0xff);
}

static void mfp_clr_IPR(int inum)
{
  int t;

  t = IPR&(~(1<<inum));
  mfpreg[IPRA] = (t>>8);
  mfpreg[IPRB] = (t&0xff);
}

static void mfp_set_ISR(int inum)
{
  int t;

  TRACE("Set ISR %d", inum);
  t = ISR|(1<<inum);
  mfpreg[ISRA] = (t>>8);
  mfpreg[ISRB] = (t&0xff);
}

static void mfp_clr_ISR(int inum)
{
  int t;

  TRACE("Clear ISR %d", inum);
  t = ISR&(~(1<<inum));
  mfpreg[ISRA] = (t>>8);
  mfpreg[ISRB] = (t&0xff);
}

static int mfp_higher_ISR(int inum)
{
  int t;

  t = ~((1<<(inum+1))-1);

  return (ISR & t);
}

void mfp_set_pending(int inum)
{
  TRACE("Set pending %d", inum);
  if(!(IER & (1<<inum))) {
    return;
  }
  mfp_set_IPR(inum);
}

static void mfp_do_interrupt(int inum)
{
  int vec,tmp;

  if(mfpreg[ISRB]&0x40) {
    if((cpu->sr&0x700) < 0x600) {
      DEBUG("We're out of MFP interrupt with ISR still high");
    }
  }
  
  if(!(IER & (1<<inum))) {
    mfp_clr_IPR(inum);
    return;
  }

  if((tmp = mfp_higher_ISR(inum))) {
    return;
  }

  if(!(IMR & (1<<inum))) {
    return;
  }
  if(IPL >= 6) {
    return;
  }

  vec = (mfpreg[VR]&0xf0);
  vec += inum;
  if(mfpreg[VR]&0x08)
    mfp_set_ISR(inum);
  else {
    mfp_clr_ISR(inum);
  }
  mfp_clr_IPR(inum);

  TRACE("Interrupt %d", vec);
  cpu_set_interrupt(IPL_MFP, vec);
}

static void mfp_do_interrupts(struct cpu *cpu)
{
  long mfpcycle;
  long tmpcpu;
  int i;

  tmpcpu = cpu->cycle - lastcpucnt;
  if(tmpcpu < 0) {
    tmpcpu += MAX_CYCLE;
  }

  mfpcycle = (tmpcpu * 96);

  update_timer(0, mfpcycle);
  update_timer(1, mfpcycle);
  update_timer(2, mfpcycle);
  update_timer(3, mfpcycle);

  for(i=15;i>=0;i--) {
    if(IPR&(1<<i)) {
      mfp_do_interrupt(i);
    }
  }

  lastcpucnt = cpu->cycle;
}

void mfp_do_timerb_event(struct cpu *cpu)
{
  if((mfpreg[TBCR]&0xf) != 8) return;
  timercnt[1]--;
  if(!timercnt[1]) {
    timercnt[1] = mfpreg[TBDR];
    if(mfpreg[IERA]&1) {
      //      printf("DEBUG: should do event-interrupt here for Timer B.\n");
      mfp_set_pending(INT_TIMERB);
    }
  }
}
