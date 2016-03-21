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

static int plane = 0;

static WORD IR[4];
static WORD RR[4];
int clock = 0;
int reload = 0;
int blank = 0;
int de = 0;

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
  DEBUG("Resolution %d", data);
  resolution = data;
  res = res_data[data&3];
  glue_set_resolution(data & 3);

  // This is a very ugly hack to make medium and high resolution work.
  // It's absolutely not what a real shifter does when switching
  // resolution.  Also, I have only a vague idea why this works.
  if(data == 1)
    reload = 20;
  else if(data == 2)
    reload = 24;
}

static BYTE shifter_read_byte(LONG addr)
{
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
  reload--;
}

static void shifter_draw_medium(void)
{
  int i;
  for (i = 0; i < 2; i++) {
    int c = shift_out();
    shifter_draw(palette_r[c], palette_g[c], palette_b[c]);
    reload--;
  }
}

static void shifter_draw_high(void)
{
  int i;
  for (i = 0; i < 4; i++) {
    int c = shift_out();
    screen_draw(palette_m[c], palette_m[c], palette_m[c]);
    reload--;
  }
}

void shifter_clock(void)
{
  res.draw();

  if(reload == 0) {
    if(res.bitplanes == 2) {
      RR[0] = RR[2];
      RR[1] = RR[3];
      RR[2] = 0;
      RR[3] = 0;
    } else if(res.bitplanes == 1) {
      RR[0] = RR[1];
      RR[1] = RR[2];
      RR[2] = RR[3];
      RR[3] = (palette_m[0] ? 0xffff : 0);
    }
    reload = 16;
    TRACE("Reload: %04x %04x %04x", RR[0], RR[1], RR[2], RR[3]);
  }

  clock++;
}

void shifter_load(WORD data)
{
  TRACE("Load %04x", data);
  IR[plane++] = data;
  if(plane == 4) {
    if(de)
      memcpy(RR, IR, sizeof RR);
    plane = 0;
  }
}

void shifter_de(int x)
{
  de = x;
}

void shifter_blank(int x)
{
  blank = x;
}
