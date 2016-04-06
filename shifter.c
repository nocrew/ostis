/*
 * SHIFTER	Connected to
 *
 * LOAD		MMU:DCYC
 * CS		MMU:CMPCS
 * R/W		BUS:R/W
 * R1-R5	BUS:A1-A5
 * DE		MMU:DE, GLUE:DE, MFP:TBI
 * D0-D15	RAM
 * GND		GND
 * XTL0		32 MHz
 * XTL1		GND
 * RGB		SCREEN:RGB
 * MONO		SCREEN:MONO
 * 16 MHz	?
 */

#include "common.h"
#include "cpu.h"
#include "screen.h"
#include "mmu.h"
#include "state.h"
#include "diag.h"
#include "glue.h"

#define SHIFTERSIZE 64
#define SHIFTERBASE 0xff8240

static void shifter_draw_low(void);
static void shifter_draw_medium(void);
static void shifter_draw_high(void);

struct resolution_data {
  void (*draw)(void);
  int bitplanes;
};

static struct resolution_data res_data[] = {
  {
    // Low resolution
    .draw = shifter_draw_low,
    .bitplanes = 4,
  },
  {
    // Medium resolution
    .draw = shifter_draw_medium,
    .bitplanes = 2,
  },
  {
    // High resolution
    .draw = shifter_draw_high,
    .bitplanes = 1,
  },
  {
    // Whoops, bad resolution
    0
  }
};

static WORD IR[4];
static WORD RR[4];
static int ir_data = -1;
static unsigned ir_clock = 0;
static unsigned rr_clock = 0;
static int blank = 0;
static char de_history[16]; // Better go with power of two.

static long palette_r[16];
static long palette_g[16];
static long palette_b[16];
static long palette_m[2];

static WORD stpal[16];
static BYTE resolution; /* Low, medium or high resolution. */
static struct resolution_data res;

HANDLE_DIAGNOSTICS(shifter)

static void set_palette(int pnum, int value, int part)
{
  int c;

  switch(part) {
  case 1: /* Low byte, only Red */
    c = (value&0x7)<<5;
    palette_r[pnum] = c;
    break;
  case 2: /* High byte, only Green and Blue */
    c = (value&0x7)<<5;
    palette_b[pnum] = c;
    c = (value&0x70)<<1;
    palette_g[pnum] = c;
    break;
  default: /* Currently unused */
    break;
  }
}

static void shifter_set_resolution(BYTE data)
{
  TRACE("Resolution %d", data);
  resolution = data;
  res = res_data[data&3];
  glue_set_resolution(data & 3);
}

static BYTE shifter_read_byte(LONG addr)
{
  MMU_WAIT_STATES();

  switch(addr) {
  case 0xff8260:
    return resolution;
  default:
    if((addr >= 0xff8240) &&
       (addr <= 0xff825f)) {
      if(addr&1)
        return stpal[(addr-0xff8240)>>1]&0xff;
      else
        return (stpal[(addr-0xff8240)>>1]&0xff00)>>8;
    } else {
      return 0;
    }
  }
}

static WORD shifter_read_word(LONG addr)
{
  return (shifter_read_byte(addr)<<8)|shifter_read_byte(addr+1);
}

static void shifter_write_byte(LONG addr, BYTE data)
{
  WORD tmp;

  MMU_WAIT_STATES();

  switch(addr) {
  case 0xff8260:
    shifter_set_resolution(data);
    return;
  default:
    if((addr >= 0xff8240) &&
       (addr <= 0xff825f)) {
      if(addr&1) {
        tmp = stpal[(addr-0xff8240)>>1];
        stpal[(addr-0xff8240)>>1] = (tmp&0xff00)|data;
	set_palette((addr-0xff8240)>>1, data, 2);
      } else {
        tmp = stpal[(addr-0xff8240)>>1];
        stpal[(addr-0xff8240)>>1] = (tmp&0xff)|(data<<8);
	set_palette((addr-0xff8240)>>1, data, 1);
      }
      palette_m[0] = (stpal[0] & 1) ? 0xff : 0;
      palette_m[1] = ~palette_m[0];
    }
    return;
  }
}

static void shifter_write_word(LONG addr, WORD data)
{
  shifter_write_byte(addr, (data&0xff00)>>8);
  shifter_write_byte(addr+1, (data&0xff));
}

static int shifter_state_collect(struct mmu_state *state)
{
  int r;

  /* Size:
   * 
   * stpal[16]   == 16*2
   * resolution  == 1
   */

  state->size = 70;
  state->data = xmalloc(state->size);
  if(state->data == NULL) {
    return STATE_INVALID;
  }
  for(r=0;r<16;r++) {
    state_write_mem_word(&state->data[r*2], stpal[r]);
  }
  state_write_mem_byte(&state->data[16*2+4*9], resolution);
  
  return STATE_VALID;
}

static void shifter_state_restore(struct mmu_state *state)
{
  int r;
  for(r=0;r<16;r++) {
    stpal[r] = state_read_mem_word(&state->data[r*2]);
    set_palette(r, stpal[r]>>8, 1);
    set_palette(r, stpal[r], 2);
  }
  resolution = state_read_mem_byte(&state->data[16*2+4*9]);
}

void shifter_init()
{
  struct mmu *shifter;

  shifter = mmu_create("SHFT", "Shifter");
  
  shifter->start = SHIFTERBASE;
  shifter->size = SHIFTERSIZE;
  shifter->read_byte = shifter_read_byte;
  shifter->read_word = shifter_read_word;
  shifter->write_byte = shifter_write_byte;
  shifter->write_word = shifter_write_word;
  shifter->state_collect = shifter_state_collect;
  shifter->state_restore = shifter_state_restore;
  shifter->diagnostics = shifter_diagnostics;

  shifter_set_resolution(0);

  mmu_register(shifter);

  memset(de_history, 0, sizeof de_history);
}

static void shifter_draw(int r, int g, int b)
{
  if(blank)
    screen_draw(0, 0, 0);
  else
    screen_draw(r, g, b);
}

static int shift_out(void)
{
  int i, c = 0, x = 1;
  for(i = 0; i < res.bitplanes; i++) {
    if(RR[i] & 0x8000)
      c += x;
    x <<= 1;
    RR[i] <<= 1;
  }
  return c;
}

static void shifter_draw_low(void)
{
  int c = shift_out();
  shifter_draw(palette_r[c], palette_g[c], palette_b[c]);
  shifter_draw(palette_r[c], palette_g[c], palette_b[c]);
}

static void shifter_draw_medium(void)
{
  int i;
  for (i = 0; i < 2; i++) {
    int c = shift_out();
    shifter_draw(palette_r[c], palette_g[c], palette_b[c]);
  }
}

static void shifter_draw_high(void)
{
  int i;
  for (i = 0; i < 4; i++) {
    int c = shift_out();
    screen_draw(palette_m[c], palette_m[c], palette_m[c]);
  }
}

static void load_ir(void)
{
  if (ir_data >= 0) {
    CLOCK("Load IR%d: %04x", (ir_clock >> 2) & 3, ir_data);
    IR[(ir_clock >> 2) & 3] = ir_data;
    ir_data = -1;
  }
}

static void load_rr(void)
{
  memcpy(RR, IR, sizeof RR);
  CLOCK("Load RR: %04x %04x %04x %04x", RR[0], RR[1], RR[2], RR[3]);
}

void shift_rr(void)
{
  if(res.bitplanes == 2 && (rr_clock & 15) == 4) {
    RR[0] = RR[2];
    RR[1] = RR[3];
    RR[2] = 0;
    RR[3] = 0;
    CLOCK("Shift RR: %04x %04x <- %04x %04x", RR[0], RR[1], RR[2], RR[3]);
  } else if(res.bitplanes == 1) {
    RR[0] = RR[1];
    RR[1] = RR[2];
    RR[2] = RR[3];
    RR[3] = (palette_m[0] ? 0xffff : 0);
    CLOCK("Shift RR: %04x <- %04x %04x %04x", RR[0], RR[1], RR[2], RR[3]);
  }
}

#define CASE(N, X) case N: X; break

/* SHIFTER STATE MACHINE
 *
 * This implementation is split in two parts: one handles the rotating
 * registers, RR, and the other handles the internal registers, IR.
 *
 * The RR part runs continuously.  The registers shift out pixels
 * every clock cycle.  In high and medium resolution, the higher-
 * numbered registers are copied to the lower-numbered every 4 or 8
 * cycles, respectively.
 *
 * The IR part only runs when the DE signal from the GLUE is active.
 * To make this work, there must be a 5-cycle delay from the start of
 * DE until the IR parts starts running, and a equal delay from the
 * end of DE until he IR part pauses.
 *
 * The two parts must be synchronised, so that a copy from the IR
 * registers to the RR registers happen at the right time.
 */

void shifter_clock(void)
{
  // RR part: check every four cycles whether copying needs to happen.
  if((rr_clock & 3) == 0) {
    shift_rr();
  }

  // IR part: check every four cycles whether the MMU has provided any
  // data and raised LOAD.  Every sixteen cycles, copy all IR registers
  // to RR.
  switch(ir_clock & 15) {
    CASE( 3, load_ir());
    CASE( 7, load_ir());
    CASE(11, load_ir());
    CASE(15, load_ir(); load_rr());
  }

  res.draw();

  // Check if the DE was active five cycles back; if so increment the
  // RR clock counter.
  if(de_history[(rr_clock - 5) % sizeof de_history]) {
    ir_clock++;
  }
  rr_clock++;
}

void shifter_load(WORD data)
{
  CLOCK("LOAD: %04x", data);
  ir_data = data;
}

void shifter_de(int x)
{
  // Save the state if the DE signal in a circular buffer.
  de_history[rr_clock % sizeof de_history] = x;
}

void shifter_blank(int x)
{
  blank = x;
}
