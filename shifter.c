#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL.h>
#include <fcntl.h>
#include <sys/time.h>
#include "common.h"
#include "cpu.h"
#include "mfp.h"
#include "screen.h"
#include "mmu.h"
#include "ram.h"
#include "state.h"
#include "diag.h"
#include "glue.h"

#define SHIFTERSIZE 64
#define SHIFTERBASE 0xff8240

#define HBLSIZE 512
#define HBLPRE 64
#define HBLSCR 320
#define HBLPOST 88

#define VBLSIZE 313
#define VBLPRE 64
#define VBLSCR 200
#define VBLPOST 40

static struct resolution_data res_data[] = {
  {
    // Low resolution
    .screen_cycles = HBLSIZE*VBLSIZE,
    .hblsize = HBLSIZE,
    .hblpre = HBLPRE,
    .hblscr = HBLSCR,
    .hblpost = HBLPOST,
    .vblsize = VBLSIZE,
    .vblpre = VBLPRE,
    .vblscr = VBLSCR,
    .voff_shift = 1,
  },
  {
    // Medium resolution
    .screen_cycles = HBLSIZE*VBLSIZE,
    .hblsize = HBLSIZE,
    .hblpre = HBLPRE,
    .hblscr = HBLSCR,
    .hblpost = HBLPOST,
    .vblsize = VBLSIZE,
    .vblpre = VBLPRE,
    .vblscr = VBLSCR,
    .voff_shift = 1,
  },
  {
    // High resolution
    .screen_cycles = 224*501,
    .hblsize = 224,
    .hblpre = 30,
    .hblscr = 160,
    .hblpost = 30,
    .vblsize = 501,
    .vblpre = 50,
    .vblscr = 400,
    .voff_shift = 2,
  },
  {
    // Whoops, bad resolution
    0
  }
};

#define BORDERTOP 28
#define BORDERBOTTOM 38
#define BORDERLEFT 32
#define BORDERRIGHT 32


static long linenum = 0;
static long linecnt;
static long vsynccnt = 0; /* 160256 cycles in one screen */
static long hsynccnt; /* Offset for first hbl interrupt */
static long lastcpucnt;

static long palette_r[16*2]; /* x2 for slightly different border col */
static long palette_g[16*2]; /* x2 for slightly different border col */
static long palette_b[16*2]; /* x2 for slightly different border col */

static WORD stpal[16];
static BYTE resolution; /* Low, medium or high resolution. */
static struct resolution_data res;

static int vblpre = 0;
static int vblscr = 0;
static int hblpre = 0;
static int hblscr = 0;
static int framecnt;
static int64_t last_framecnt_usec;
static int usecs_per_framecnt_interval = 1;

static int ppm_fd;
static unsigned char *rgbimage;

static int vbl_triggered = 0;
static int hbl_triggered = 0;

HANDLE_DIAGNOSTICS(shifter)

static void set_palette(int pnum, int value, int part)
{
  int c;

  switch(part) {
  case 1: /* Low byte, only Red */
    c = (value&0x7)<<5;
    palette_r[pnum] = c;
    palette_r[pnum+16] = c|0x1f; /* Brighter for border */
    break;
  case 2: /* High byte, only Green and Blue */
    c = (value&0x7)<<5;
    palette_b[pnum] = c;
    palette_b[pnum+16] = c|0x1f; /* Brighter for border */
    c = (value&0x70)<<1;
    palette_g[pnum] = c;
    palette_g[pnum+16] = c|0x1f; /* Brighter for border */
    break;
  default: /* Currently unused */
    break;
  }
}

static void shifter_set_resolution(BYTE data)
{
  resolution = data;

  res = res_data[data&3];

  if(ppmoutput || crop_screen)
    res.border = 0;

  glue_set_resolution(data & 3);
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
   * linenum     == 4
   * linecnt     == 4
   * vsynccnt    == 4
   * hsynccnt    == 4
   * lastcpucnt  == 4
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
  state_write_mem_long(&state->data[16*2+4*3], linenum);
  state_write_mem_long(&state->data[16*2+4*4], linecnt);
  state_write_mem_long(&state->data[16*2+4*5], vsynccnt);
  state_write_mem_long(&state->data[16*2+4*6], hsynccnt);
  state_write_mem_long(&state->data[16*2+4*8], lastcpucnt);
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
  linenum = state_read_mem_long(&state->data[16*2+4*3]);
  linecnt = state_read_mem_long(&state->data[16*2+4*4]);
  vsynccnt = state_read_mem_long(&state->data[16*2+4*5]);
  hsynccnt = state_read_mem_long(&state->data[16*2+4*6]);
  lastcpucnt = state_read_mem_long(&state->data[16*2+4*8]);
  resolution = state_read_mem_byte(&state->data[16*2+4*9]);
}

void shifter_build_ppm()
{
  int x,y,c;
  char header[80];
  unsigned char frame[384*288*3];

  c = 0;

  for(y=0;y<288;y++) {
    for(x=0;x<384;x++) {
      frame[c*3+0] = rgbimage[((y+12)*512+x+32)*6+2];
      frame[c*3+1] = rgbimage[((y+12)*512+x+32)*6+1];
      frame[c*3+2] = rgbimage[((y+12)*512+x+32)*6+0];
      c++;
    }
  }

  sprintf(header, "P6\n%d %d\n255\n", 384, 288);
  if(write(ppm_fd, header, strlen(header)) != strlen(header))
    WARNING(write);
  if (write(ppm_fd, frame, 384*288*3) != 384*288*3)
    WARNING(write);
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

  linecnt = res.hblsize+432; /* Cycle count per line, for timer-b event */
  hsynccnt = res.hblsize+16; /* Offset for first hbl interrupt */

  mmu_register(shifter);

  rgbimage = screen_pixels();
  if(ppmoutput) {
    ppm_fd = open("ostis.ppm", O_WRONLY|O_CREAT|O_TRUNC, 0644);
  }
  framecnt = 0;
}

int64_t last_vsync_ticks = 0;

static int64_t usec_count() {
  static struct timeval tv;

  gettimeofday(&tv,NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

void shifter_do_interrupts(struct cpu *cpu, int noint)
{
  long tmpcpu;
  int64_t current_ticks = 0;
  int64_t remaining = 0;

  tmpcpu = cpu->cycle - lastcpucnt;
  if(tmpcpu < 0) tmpcpu += MAX_CYCLE;

  vsynccnt -= tmpcpu;
  hsynccnt -= tmpcpu;
  linecnt -= tmpcpu;
  
  /* VBL Interrupt */
  if(vsynccnt < 0) {
    vblpre = res.vblpre;
    vblscr = res.vblscr;
    vsynccnt += res.screen_cycles;
    linenum = 0;
    hsynccnt += res.hblsize;
    if(ppmoutput) {
      shifter_build_ppm();
    }
    if(vsync_delay) {
      current_ticks = usec_count();
      remaining = current_ticks - last_vsync_ticks;
      while(remaining < 20000) {
	//	printf("DEBUG: Remaining: %ld\n", remaining);
	current_ticks = usec_count();
	remaining = current_ticks - last_vsync_ticks;
      }
      last_vsync_ticks = current_ticks;
    }
    screen_swap(SCREEN_NORMAL);
    vbl_triggered = 1;
    framecnt++;
    if((framecnt&0x3f) == 0) {
      current_ticks = usec_count();
      usecs_per_framecnt_interval = current_ticks - last_framecnt_usec;
      last_framecnt_usec = current_ticks;
    }
  }

  /* HBL Interrupt */
  if(hsynccnt < 0) {
    hblpre = res.hblpre;
    hblscr = res.hblscr;
    hsynccnt += res.hblsize;
    hbl_triggered = 1;
    cpu_set_interrupt(IPL_HBL, IPL_NO_AUTOVECTOR); /* This _should_ work, but probably won't */
  }

  if(!noint && (IPL < 4) && vbl_triggered) {
    vbl_triggered = 0;
    cpu_set_interrupt(IPL_VBL, IPL_NO_AUTOVECTOR); /* Set VBL interrupt as pending */
  }

  if(!noint && (IPL < 2) && hbl_triggered) {
    hbl_triggered = 0;
    cpu_set_interrupt(IPL_HBL, IPL_NO_AUTOVECTOR); /* Set HBL interrupt as pending */
  }
  
  /* Line Interrupt */
  if(linecnt < 0) {
    linecnt += res.hblsize;
    if((linenum >= vblpre) && (linenum < (vblpre+vblscr)))
      mfp_do_timerb_event(cpu);
    linenum++;
  }

  lastcpucnt = cpu->cycle;
}

int shifter_get_vsync()
{
  return res.screen_cycles-vsynccnt;
}

int shifter_framecnt(int c)
{
  if(c == -1) {
    framecnt = 0;
  }
  return framecnt;
}

float shifter_fps()
{
  if(usecs_per_framecnt_interval) {
    return 1000000*64.0/usecs_per_framecnt_interval;
  } else {
    return 0;
  }
}

static int rasterpos;

static int get_color(WORD *data)
{
  int i, c = 0;
  for(i = 0; i < 4; i++) {
    c <<= 1;
    if(data[i] & 0x8000)
      c += 1;
    data[i] <<= 1;
  }
  return c;
}

static void shifter_draw(WORD *data)
{
  int i, c;

  for (i = 0; i < 16; i++) {
    c = get_color(data);
    rgbimage[rasterpos++] = palette_r[c];
    rgbimage[rasterpos++] = palette_g[c];
    rgbimage[rasterpos++] = palette_b[c];
    rgbimage[rasterpos++] = palette_r[c];
    rgbimage[rasterpos++] = palette_g[c];
    rgbimage[rasterpos++] = palette_b[c];
  }
}

void shifter_load(WORD data)
{
  static WORD reg[4];
  static int i = 0;
  TRACE("Load %04x", data);

  reg[3-i] = data;
  i++;
  if(i == 4) {
    i = 0;
    shifter_draw(reg);
  }
}

void shifter_border(void)
{
  TRACE("Border");

  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;

  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;

  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;

  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;
  rgbimage[rasterpos++] = 0xff;

  if(rasterpos >= 6*160256)
    rasterpos = 0;
}
