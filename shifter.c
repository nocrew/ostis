#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL_byteorder.h>
#include <fcntl.h>
#include "common.h"
#include "cpu.h"
#include "mfp.h"
#include "screen.h"
#include "mmu.h"
#include "state.h"

#define PPMOUTPUT 0

#define SHIFTERSIZE 128
#define SHIFTERBASE 0xff8200

#define HBLSIZE 512
#define HBLPRE 64
#define HBLSCR 320
#define HBLPOST 88

#define VBLSIZE 313
#define VBLPRE 64
#define VBLSCR 200
#define VBLPOST 40

#define SCR_BYTES_PER_LINE 160

#define BORDERTOP 28
#define BORDERBOTTOM 38
#define BORDERLEFT 32
#define BORDERRIGHT 32


static long linenum = 0;
static long linecnt = HBLSIZE+432; /* Cycle count per line, for timer-b event */
static long vsynccnt = VBLSIZE*HBLSIZE; /* 160256 cycles in one screen */
static long hsynccnt = HBLSIZE+16; /* Offset for first hbl interrupt */
static long lastrasterpos = 0;
static long lastcpucnt;

static long palette_r[16*2]; /* x2 for slightly different border col */
static long palette_g[16*2]; /* x2 for slightly different border col */
static long palette_b[16*2]; /* x2 for slightly different border col */

static WORD stpal[16];
static LONG curaddr; /* Current address used for display */
static LONG scraddr; /* Address for next VBL */
static LONG scrptr;  /* Actual screen pointer, read only from emulator */
static BYTE syncreg; /* 50/60Hz + Internal/External Sync flags */
static BYTE resolution; /* Low, medium or high resolution. Only low now. */

static int vblpre = VBLPRE;
static int vblscr = VBLSCR;
static int hblpre = HBLPRE;
static int hblscr = HBLSCR;
static int scr_bytes_per_line = SCR_BYTES_PER_LINE;
static int framecnt;

static int ppm_fd;
static unsigned char *rgbimage;

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

static void set_pixel_low(int rasterpos, int pnum)
{
  //  if(pnum == 0) return;

#if (SDL_BYTEORDER == SDL_BIG_ENDIAN) || DEBUG
  rgbimage[rasterpos*3+0] = palette_r[pnum];
  rgbimage[rasterpos*3+1] = palette_g[pnum];
  rgbimage[rasterpos*3+2] = palette_b[pnum];
#else
  rgbimage[rasterpos*3+2] = palette_r[pnum];
  rgbimage[rasterpos*3+1] = palette_g[pnum];
  rgbimage[rasterpos*3+0] = palette_b[pnum];
#endif
}

static void set_pixel_medium(int rasterpos, int pnum)
{
  int r,g,b;
  int c1,c2;

  c1 = (pnum>>16);
  c2 = pnum&0xffff;

  r = (palette_r[c1]+palette_r[c2])/2;
  g = (palette_g[c1]+palette_g[c2])/2;
  b = (palette_b[c1]+palette_b[c2])/2;

#if (SDL_BYTEORDER == SDL_BIG_ENDIAN) || DEBUG
  rgbimage[rasterpos*3+0] = r;
  rgbimage[rasterpos*3+1] = g;
  rgbimage[rasterpos*3+2] = b;
#else
  rgbimage[rasterpos*3+2] = r;
  rgbimage[rasterpos*3+1] = g;
  rgbimage[rasterpos*3+0] = b;
#endif
}

static void set_pixel(int rasterpos, int pnum)
{
  if(resolution&1) {
    return set_pixel_medium(rasterpos, pnum);
  } else {
    return set_pixel_low(rasterpos, pnum);
  }
}

static void fill_16pxl(int rasterpos, int pnum)
{
  int i;

  for(i=0;i<16;i++) {
    set_pixel(rasterpos+i, pnum);
  }
}

static int get_pixel_low(int videooffset, int pxlnum)
{
  int c;
  static int lastpos = 0;
  static WORD d[4];

  if((curaddr+videooffset*2) != lastpos) {
    d[3] = mmu_read_word_print(curaddr+videooffset*2+0);
    d[2] = mmu_read_word_print(curaddr+videooffset*2+2);
    d[1] = mmu_read_word_print(curaddr+videooffset*2+4);
    d[0] = mmu_read_word_print(curaddr+videooffset*2+6);
    lastpos = curaddr+videooffset*2;
  }
  
  c = ((((d[0]>>(15-pxlnum))&1)<<3)|
       (((d[1]>>(15-pxlnum))&1)<<2)|
       (((d[2]>>(15-pxlnum))&1)<<1)|
       (((d[3]>>(15-pxlnum))&1)));
  return c;
}

static int get_pixel_medium(int videooffset, int pxlnum)
{
  int c,c1,c2;
  static int lastpos = 0;
  static WORD d[4];

  if((curaddr+videooffset) != lastpos) {
    d[3] = mmu_read_word_print(curaddr+videooffset*2+0);
    d[2] = mmu_read_word_print(curaddr+videooffset*2+2);
    d[1] = mmu_read_word_print(curaddr+videooffset*2+4);
    d[0] = mmu_read_word_print(curaddr+videooffset*2+6);
    lastpos = curaddr+videooffset;
  }

  if(pxlnum < 8) {
    c1 = ((((d[2]>>(15-(pxlnum*2)))&1)<<1)|
	  (((d[3]>>(15-(pxlnum*2)))&1)));
    c2 = ((((d[2]>>(15-(pxlnum*2+1)))&1)<<1)|
	  (((d[3]>>(15-(pxlnum*2+1)))&1)));
  } else {
    pxlnum -= 8;
    c1 = ((((d[0]>>(15-(pxlnum*2)))&1)<<1)|
	  (((d[1]>>(15-(pxlnum*2)))&1)));
    c2 = ((((d[0]>>(15-(pxlnum*2+1)))&1)<<1)|
	  (((d[1]>>(15-(pxlnum*2+1)))&1)));
  }

  c = (c1<<16)|c2;

  return c;
}

static int get_pixel(int videooffset, int pxlnum)
{
  if(resolution&1) {
    return get_pixel_medium(videooffset, pxlnum);
  } else {
    return get_pixel_low(videooffset, pxlnum);
  }
}

static void set_16pxl(int rasterpos, int videooffset)
{
  int i,c;
  WORD d[4];

  d[3] = mmu_read_word_print(curaddr+videooffset*2+0);
  d[2] = mmu_read_word_print(curaddr+videooffset*2+2);
  d[1] = mmu_read_word_print(curaddr+videooffset*2+4);
  d[0] = mmu_read_word_print(curaddr+videooffset*2+6);
  
  for(i=15;i>=0;i--) {
    c = ((((d[0]>>i)&1)<<3)|
         (((d[1]>>i)&1)<<2)|
         (((d[2]>>i)&1)<<1)|
         (((d[3]>>i)&1)));
    set_pixel(rasterpos+(15-i), c);
  }
}

int shifter_on_display(int rasterpos)
{
  int line,linepos;

  line = rasterpos/HBLSIZE;
  linepos = rasterpos%HBLSIZE;
  
  if((line < vblpre) || (line >= (vblpre+vblscr)) ||
     (linepos < hblpre) || (linepos >= (hblpre+hblscr))) {
    return 0;
  }
  return 1;
}

static long get_videooffset(int rasterpos)
{
  int line,linepos,voff;

  line = rasterpos/HBLSIZE;
  linepos = rasterpos%HBLSIZE;

  voff = (line-vblpre)*scr_bytes_per_line/2;
  voff += ((linepos-hblpre)/16)*4; /* Mainly stolen from hatari */

  return voff;
}

static void gen_scrptr(int rasterpos)
{
  int line,linepos,voff;

  line = rasterpos/HBLSIZE;
  linepos = rasterpos%HBLSIZE;

  if(shifter_on_display(rasterpos)) {
    voff = (line-vblpre)*scr_bytes_per_line;
    voff += ((linepos-hblpre+15)>>1)&(~1); /* Mainly stolen from hatari */
    scrptr = curaddr + voff;
  } else {
    if(line < vblpre) {
      scrptr = curaddr;
    } else if(line >= (vblpre+vblscr)) {
      scrptr = curaddr + scr_bytes_per_line*vblscr;
    } else {
      if(linepos < hblpre) {
	scrptr = curaddr + (line-vblpre)*scr_bytes_per_line;
      } else if(linepos >= (hblpre+hblscr)) {
	scrptr = curaddr + (line-vblpre+1)*scr_bytes_per_line;
      }
    }
  }
}

static void shifter_gen_pixel(int rasterpos)
{
  int linepos;

  linepos = rasterpos%HBLSIZE;

  if(shifter_on_display(rasterpos)) {
    set_pixel(rasterpos,
	      get_pixel(get_videooffset(rasterpos),
			(linepos-hblpre)&15));
  } else {
#if PPMOUTPUT
    set_pixel(rasterpos, 0); /* Background in border */
#else
    set_pixel(rasterpos, 16); /* Background in border */
#endif
  }
}

static void shifter_gen_16pxl(int rasterpos)
{
  if(shifter_on_display(rasterpos)) {
    fill_16pxl(rasterpos, 0);
  } else {
    set_16pxl(rasterpos, get_videooffset(rasterpos));
  }
}

static void shifter_gen_picture(int rasterpos)
{
  int i;

  if((rasterpos - lastrasterpos) < 0) return;

  for(i=lastrasterpos; i<=rasterpos; i++) {
    shifter_gen_pixel(i);
  }
  lastrasterpos = i;
}

void shifter_build_image(int debug)
{
  
}

static BYTE shifter_read_byte(LONG addr)
{
  switch(addr) {
  case 0xff8201:
    return (scraddr&0xff0000)>>16;
  case 0xff8203:
    return (scraddr&0xff00)>>8;
  case 0xff8205:
    gen_scrptr(VBLSIZE*HBLSIZE-vsynccnt);
    return (scrptr&0xff0000)>>16;
  case 0xff8207:
    gen_scrptr(VBLSIZE*HBLSIZE-vsynccnt);
    return (scrptr&0xff00)>>8;
  case 0xff8209:
    gen_scrptr(VBLSIZE*HBLSIZE-vsynccnt);
    return scrptr&0xff;
  case 0xff820a:
    return syncreg;
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

static LONG shifter_read_long(LONG addr)
{
  return ((shifter_read_byte(addr)<<24)|
          (shifter_read_byte(addr+1)<<16)|
          (shifter_read_byte(addr+2)<<8)|
          (shifter_read_byte(addr+3)));
}

static void shifter_write_byte(LONG addr, BYTE data)
{
  WORD tmp;

  switch(addr) {
  case 0xff8201:
    scraddr = (scraddr&0xff00)|(data<<16);
    return;
  case 0xff8203:
    scraddr = (scraddr&0xff0000)|(data<<8);
    return;
  case 0xff820a:
    if((160256-vsynccnt) < (HBLSIZE*VBLPRE)) {
      vblpre = VBLPRE-BORDERTOP;
      vblscr = VBLSCR+BORDERTOP;
    } else if((160256-vsynccnt) > ((HBLSIZE*(vblpre+vblscr))-HBLPOST)) {
      if(vblscr != VBLSCR) {
	vblscr = BORDERTOP;
      }
      vblscr = VBLSCR+BORDERBOTTOM;
    }
    syncreg = data;
    return;
  case 0xff8260:
    resolution = data;
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
      return;
    } else {
      return;
    }
  }
}

static void shifter_write_word(LONG addr, WORD data)
{
  if((addr >= 0xff8240) && (addr <= 0xff825f))
    shifter_gen_picture(VBLSIZE*HBLSIZE-vsynccnt);
  shifter_write_byte(addr, (data&0xff00)>>8);
  shifter_write_byte(addr+1, (data&0xff));
}

static void shifter_write_long(LONG addr, LONG data)
{
  if((addr >= 0xff8240) && (addr <= 0xff825f)) {
    shifter_gen_picture(VBLSIZE*HBLSIZE-vsynccnt);
  }
  shifter_write_byte(addr, (data&0xff000000)>>24);
  shifter_write_byte(addr+1, (data&0xff0000)>>16);
  shifter_write_byte(addr+2, (data&0xff00)>>8);
  shifter_write_byte(addr+3, (data&0xff));
}

static int shifter_state_collect(struct mmu_state *state)
{
  int r;

  /* Size:
   * 
   * stpal[16]   == 16*2
   * curaddr     == 4
   * scraddr     == 4
   * scrptr      == 4
   * linenum     == 4
   * linecnt     == 4
   * vsynccnt    == 4
   * hsynccnt    == 4
   * lastrasterpos == 4
   * lastcpucnt  == 4
   * resolution  == 1
   * syncreg     == 1
   */

  state->size = 70;
  state->data = (char *)malloc(state->size);
  if(state->data == NULL) {
    return STATE_INVALID;
  }
  for(r=0;r<16;r++) {
    state_write_mem_word(&state->data[r*2], stpal[r]);
  }
  state_write_mem_long(&state->data[16*2], curaddr);
  state_write_mem_long(&state->data[16*2+4*1], scraddr);
  state_write_mem_long(&state->data[16*2+4*2], scrptr);
  state_write_mem_long(&state->data[16*2+4*3], linenum);
  state_write_mem_long(&state->data[16*2+4*4], linecnt);
  state_write_mem_long(&state->data[16*2+4*5], vsynccnt);
  state_write_mem_long(&state->data[16*2+4*6], hsynccnt);
  state_write_mem_long(&state->data[16*2+4*7], lastrasterpos);
  state_write_mem_long(&state->data[16*2+4*8], lastcpucnt);
  state_write_mem_byte(&state->data[16*2+4*9], resolution);
  state_write_mem_byte(&state->data[16*2+4*9+1], syncreg);
  
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
  curaddr = state_read_mem_long(&state->data[16*2]);
  scraddr = state_read_mem_long(&state->data[16*2+4*1]);
  scrptr = state_read_mem_long(&state->data[16*2+4*2]);
  linenum = state_read_mem_long(&state->data[16*2+4*3]);
  linecnt = state_read_mem_long(&state->data[16*2+4*4]);
  vsynccnt = state_read_mem_long(&state->data[16*2+4*5]);
  hsynccnt = state_read_mem_long(&state->data[16*2+4*6]);
  lastrasterpos = state_read_mem_long(&state->data[16*2+4*7]);
  lastcpucnt = state_read_mem_long(&state->data[16*2+4*8]);
  resolution = state_read_mem_byte(&state->data[16*2+4*9]);
  syncreg = state_read_mem_byte(&state->data[16*2+4*9+1]);
}

void shifter_build_ppm()
{
  int x,y,c;
  char header[80];
  unsigned char frame[384*288*3];

  c = 0;

  for(y=0;y<288;y++) {
    for(x=0;x<384;x++) {
      frame[c*3+0] = rgbimage[((y+12)*512+x+32)*3+0];
      frame[c*3+1] = rgbimage[((y+12)*512+x+32)*3+1];
      frame[c*3+2] = rgbimage[((y+12)*512+x+32)*3+2];
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

  shifter = (struct mmu *)malloc(sizeof(struct mmu));
  if(!shifter) {
    return;
  }
  
  shifter->start = SHIFTERBASE;
  shifter->size = SHIFTERSIZE;
  memcpy(shifter->id, "SHFT", 4);
  shifter->name = strdup("Shifter");
  shifter->read_byte = shifter_read_byte;
  shifter->read_word = shifter_read_word;
  shifter->read_long = shifter_read_long;
  shifter->write_byte = shifter_write_byte;
  shifter->write_word = shifter_write_word;
  shifter->write_long = shifter_write_long;
  shifter->state_collect = shifter_state_collect;
  shifter->state_restore = shifter_state_restore;

  mmu_register(shifter);

  rgbimage = screen_pixels();
#if PPMOUTPUT
  ppm_fd = open("ostis.ppm", O_WRONLY|O_CREAT|O_TRUNC, 0644);
#endif
  framecnt = 0;
}

void shifter_do_interrupts(struct cpu *cpu, int noint)
{
  long tmpcpu;

  tmpcpu = cpu->cycle - lastcpucnt;
  if(tmpcpu < 0) tmpcpu += MAX_CYCLE;

  vsynccnt -= tmpcpu;
  hsynccnt -= tmpcpu;
  linecnt -= tmpcpu;
  
  /* VBL Interrupt */
  //  shifter_gen_picture(VBLSIZE*HBLSIZE-vsynccnt);
  if(vsynccnt < 0) {
    vblpre = VBLPRE;
    vblscr = VBLSCR;
    shifter_gen_picture(VBLSIZE*HBLSIZE);
    scrptr = curaddr = scraddr;
    vsynccnt += VBLSIZE*HBLSIZE;
    linenum = 0;
    hsynccnt += HBLSIZE;
    //    hsynccnt = HBLSIZE+(160);
    //    linecnt = HBLSIZE-(HBLSIZE-HBLPRE-HBLSCR-HBLPOST);
    lastrasterpos = 0; /* Restart image building from position 0 */
#if PPMOUTPUT
    shifter_build_ppm();
#endif
    screen_swap();
    //    if(!noint && (IPL < 4))
    cpu_set_exception(28); /* Set VBL interrupt as pending */
    framecnt++;
  }

  /* HBL Interrupt */
  if(hsynccnt < 0) {
    hblpre = HBLPRE;
    hblscr = HBLSCR;
    shifter_gen_picture(VBLSIZE*HBLSIZE-vsynccnt);
    hsynccnt += HBLSIZE;
    if(!noint && (IPL < 2))
      cpu_set_exception(26); /* This _should_ work, but probably won't */

  }
  
  /* Line Interrupt */
  if(linecnt < 0) {
    linecnt += HBLSIZE;
    if((linenum >= vblpre) && (linenum < (vblpre+vblscr)))
      mfp_do_timerb_event(cpu);
#if 0
    rgbimage[(VBLSIZE*HBLSIZE-vsynccnt)*3+0] = 0xff;
    rgbimage[(VBLSIZE*HBLSIZE-vsynccnt)*3+1] = 0x00;
    rgbimage[(VBLSIZE*HBLSIZE-vsynccnt)*3+2] = 0x00;
#endif
    linenum++;
  }

  lastcpucnt = cpu->cycle;
}

int shifter_get_vsync()
{
  return VBLSIZE*HBLSIZE-vsynccnt;
}

int shifter_framecnt(int c)
{
  if(c == -1) {
    framecnt = 0;
  }
  return framecnt;
}
