#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"
#include "cpu.h"
#include "mfp.h"
#include "screen.h"
#include "mmu.h"

#define SHIFTERSIZE 256
#define SHIFTERBASE 0xff8200

#define LEFTBORDER 96
#define RIGHTBORDER (512-LEFTBORDER)
#define TOPBORDER 57
#define LOWERBORDER (313-TOPBORDER)

#define BEFOREGFX (LEFTBORDER+(TOPBORDER-1)*512+256)

/* 
   Rastersize is 512 cycles/scanline, 313 scanlines, 3 bytes/pixel 
   Only low resolution supported right now.
 */
static unsigned char rgbimage[512*314*3];
static int lastrasterpos = 0;

static WORD palette[16];
static LONG curaddr;
static LONG scraddr;
static LONG scrptr;
static BYTE syncreg;
static BYTE resolution;

static int ppmout;

static long lastcpucnt;
static long vsync;
static long hsync;
static long hline;

static long build_color(int pnum)
{
  int c,r,g,b;
  

  if(pnum > 15) exit(-4);
  c = palette[pnum];

  r = (c&0xf00)>>7;
  g = (c&0xf0)>>3;
  b = (c&0xf)<<1;

  r = (r<<4)|r;
  g = (g<<4)|g;
  b = (b<<4)|b;

  return (r<<16)|(g<<8)|b;
}

static void set_pixel(int rasterpos, int pnum)
{
  int c;
  int r,g,b;

  //  pnum = 3;

  c = palette[pnum];

  r = (c&0xf00)>>7;
  g = (c&0xf0)>>3;
  b = (c&0xf)<<1;

  r = (r<<4)|r;
  g = (g<<4)|g;
  b = (b<<4)|b;

  rgbimage[rasterpos*3+0] = r;
  rgbimage[rasterpos*3+1] = g;
  rgbimage[rasterpos*3+2] = b;
}

static void fill_16pxl(int rasterpos, int col)
{
  int i;

  for(i=0;i<16;i++) {
    set_pixel(rasterpos+i, col);
  }
}

static int get_pixel(int vmemoff, int pxl)
{
  int c;
  WORD d[4];
  
  d[3] = mmu_read_word_print(curaddr+vmemoff*2+0);
  d[2] = mmu_read_word_print(curaddr+vmemoff*2+2);
  d[1] = mmu_read_word_print(curaddr+vmemoff*2+4);
  d[0] = mmu_read_word_print(curaddr+vmemoff*2+6);
  
  c = ((((d[0]>>(15-pxl))&1)<<3)|
       (((d[1]>>(15-pxl))&1)<<2)|
       (((d[2]>>(15-pxl))&1)<<1)|
       (((d[3]>>(15-pxl))&1)));
  return c;
}

static void set_16pxl(int rasterpos, int vmemoff)
{
  int i,c;
  WORD d[4];

  d[3] = mmu_read_word_print(curaddr+vmemoff*2+0);
  d[2] = mmu_read_word_print(curaddr+vmemoff*2+2);
  d[1] = mmu_read_word_print(curaddr+vmemoff*2+4);
  d[0] = mmu_read_word_print(curaddr+vmemoff*2+6);
  
  for(i=15;i>=0;i--) {
    c = ((((d[0]>>i)&1)<<3)|
	 (((d[1]>>i)&1)<<2)|
	 (((d[2]>>i)&1)<<1)|
	 (((d[3]>>i)&1)));
    set_pixel(rasterpos+(15-i), c);
  }
}

static void gen_scrptr(int rasterpos)
{
  int vmemoff,scan,scanpos;

  rasterpos = 160256-rasterpos;

  scan = 2+((rasterpos-256)/512);
  scanpos = (rasterpos+256)%512;

  if((scan < TOPBORDER) || (scan >= LOWERBORDER) ||
     (scanpos < LEFTBORDER) || (scanpos >= RIGHTBORDER)) {
    return;
  }
  vmemoff = (scan-TOPBORDER)*160;
  vmemoff += ((scanpos-LEFTBORDER+15)>>1)&(~1);
  scrptr = curaddr + vmemoff;
}

static void shifter_gen_pixel(int rasterpos)
{
  int vmemoff,scan,scanpos;

  scan = 2+((rasterpos-256)/512);
  scanpos = (rasterpos+256)%512;

  if((scan < TOPBORDER) || (scan >= LOWERBORDER) ||
     (scanpos < LEFTBORDER) || (scanpos >= RIGHTBORDER)) {
    set_pixel(rasterpos+256, 0);
  } else {
    vmemoff = (scan-TOPBORDER)*80;
    vmemoff += ((scanpos-LEFTBORDER)/16)*4;
    set_pixel(rasterpos+256, get_pixel(vmemoff, (scanpos-LEFTBORDER)&15));
  }
}

static void shifter_gen_16pxl(int rasterpos)
{
  int vmemoff,scan,scanpos;
  scan = 2+((rasterpos-256)/512);
  scanpos = (rasterpos+256)%512;
    
  if((scan < TOPBORDER) || (scan >= LOWERBORDER) ||
     (scanpos < LEFTBORDER) || (scanpos >= RIGHTBORDER)) {
    fill_16pxl(rasterpos+256, 0);
  } else {
    vmemoff = (scan-TOPBORDER)*80;
    vmemoff += ((scanpos-LEFTBORDER)/16)*4;
    set_16pxl(rasterpos+256, vmemoff);
  }
}

static void shifter_gen_picture(long rasterpos)
{
  int i;
  int left,mid,right;

  if((rasterpos - lastrasterpos) < 0) return;

  if((rasterpos - lastrasterpos) < 30) {
    //    printf("DEBUG: raster: %ld - %d == %ld\n", rasterpos, lastrasterpos,
    //	   rasterpos - lastrasterpos);
    for(i=lastrasterpos; i<=rasterpos; i++) {
      shifter_gen_pixel(i);
    }
    lastrasterpos = i;
    return;
  }

  left = (16-(lastrasterpos%16))%16;
  right = rasterpos%16;
  mid = ((rasterpos-right-left)-((lastrasterpos+16)&0xfffffff0))/16;

  for(i=0;i<left;i++) {
    shifter_gen_pixel(i+lastrasterpos);
  }
  lastrasterpos += i;

  for(i=0;i<mid;i++) {
    shifter_gen_16pxl(i*16+lastrasterpos);
  }
  lastrasterpos += i*16;

  for(i=0;i<right;i++) {
    shifter_gen_pixel(i+lastrasterpos);
  } 
  lastrasterpos += i;
}

void shifter_build_image()
{
  screen_copyimage(rgbimage);
}

void shifter_build_ppm()
{
  static unsigned char row[1024*3];
  static char tmp[80];
  int xoff,yoff,w,h;
  int x,y;

  xoff = 8;
  yoff = 13;
  w = 480;
  h = 288;

  sprintf(tmp, "P6\n%d %d\n255\n", w, h);
  write(ppmout, tmp, strlen(tmp));
  
  for(y=0;y<h;y++) {
    for(x=0;x<w;x++) {
      row[x*3+0] = rgbimage[(y+yoff)*512*3+(x+xoff)*3+0];
      row[x*3+1] = rgbimage[(y+yoff)*512*3+(x+xoff)*3+1];
      row[x*3+2] = rgbimage[(y+yoff)*512*3+(x+xoff)*3+2];
    }
    write(ppmout, row, w*3);
  }
}

static BYTE shifter_read_byte(LONG addr)
{
  switch(addr) {
  case 0xff8201:
    return (scraddr&0xff0000)>>16;
  case 0xff8203:
    return (scraddr&0xff00)>>8;
  case 0xff8205:
    gen_scrptr(vsync);
    return (scrptr&0xff0000)>>16;
  case 0xff8207:
    gen_scrptr(vsync);
    return (scrptr&0xff00)>>8;
  case 0xff8209:
    gen_scrptr(vsync);
    return scrptr&0xff;
  case 0xff820a:
    return syncreg;
  case 0xff8260:
    return resolution;
  default:
    if((addr >= 0xff8240) &&
       (addr <= 0xff825e)) {
      if(addr&1)
	return palette[(addr-0xff8240)>>1]&0xff;
      else
	return (palette[(addr-0xff8240)>>1]&0xff00)>>8;
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
    syncreg = data;
    return;
  case 0xff8260:
    resolution = data;
    return;
  default:
    if((addr >= 0xff8240) &&
       (addr <= 0xff825f)) {
      if(addr&1) {
	tmp = palette[(addr-0xff8240)>>1];
	palette[(addr-0xff8240)>>1] = (tmp&0xff00)|data;
      } else {
	tmp = palette[(addr-0xff8240)>>1];
	palette[(addr-0xff8240)>>1] = (tmp&0xff)|(data<<8);
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
    shifter_gen_picture(160256-vsync);
  shifter_write_byte(addr, (data&0xff00)>>8);
  shifter_write_byte(addr+1, (data&0xff));
}

static void shifter_write_long(LONG addr, LONG data)
{
  if((addr >= 0xff8240) && (addr <= 0xff825f)) {
    shifter_gen_picture(160256-vsync);
  }
  shifter_write_byte(addr, (data&0xff000000)>>24);
  shifter_write_byte(addr+1, (data&0xff0000)>>16);
  shifter_write_byte(addr+2, (data&0xff00)>>8);
  shifter_write_byte(addr+3, (data&0xff));
}

void shifter_init()
{
  struct mmu *shifter;

  shifter = (struct mmu *)malloc(sizeof(struct mmu));
  if(!shifter) {
    return;
  }

  vsync = 0;
  hsync = 0;
  hline = 0;

  shifter->start = SHIFTERBASE;
  shifter->size = SHIFTERSIZE;
  shifter->name = strdup("Shifter");
  shifter->read_byte = shifter_read_byte;
  shifter->read_word = shifter_read_word;
  shifter->read_long = shifter_read_long;
  shifter->write_byte = shifter_write_byte;
  shifter->write_word = shifter_write_word;
  shifter->write_long = shifter_write_long;

  mmu_register(shifter);

  //  ppmout = open("out.ppm", O_WRONLY|O_CREAT|O_TRUNC, 0664);
  ppmout = 1; /* send everything to stdout if running... */
}

static void build_16pxl(int wcnt, WORD d[])
{
  int i,c;
  int x,y;

  y = wcnt/80;
  x = 16*((wcnt%80)/4);

  for(i=15;i>=0;i--) {
    c = ((((d[0]>>i)&1)<<3)|
	 (((d[1]>>i)&1)<<2)|
	 (((d[2]>>i)&1)<<1)|
	 (((d[3]>>i)&1)));
    //    if(dbg) printf("DEBUG: running screen_putpixel(%d, %d, %06lx (%d), 1)\n", x+(15-i), y, build_color(c), c);
    screen_putpixel(x+(15-i), y, build_color(c));
  }
}

void shifter_build_image_old()
{
  int i;
  WORD d[4];

  for(i=0;i<16000;i+=4) {
    d[3] = mmu_read_word(scraddr+i*2);
    d[2] = mmu_read_word(scraddr+i*2+2);
    d[1] = mmu_read_word(scraddr+i*2+4);
    d[0] = mmu_read_word(scraddr+i*2+6);
    build_16pxl(i, d);
  }
}

void shifter_build_ppm_16pxl(int wcnt, WORD d[], char *image)
{
  int i,c;
  int x,y;
  long col;

  y = wcnt/80;
  x = 16*((wcnt%80)/4);
  
  for(i=15;i>=0;i--) {
    c = ((((d[0]>>i)&1)<<3)|
         (((d[1]>>i)&1)<<2)|
         (((d[2]>>i)&1)<<1)|
         (((d[3]>>i)&1)));
    col = build_color(c);
    image[((15-i)*3)+0] = (col&0xff0000)>>16;
    image[((15-i)*3)+1] = (col&0xff00)>>8;
    image[((15-i)*3)+2] = (col&0xff);
  }
}

void shifter_build_ppm_old()
{
  int i;
  WORD d[4];
  static char image[192000];
  
  write(ppmout, "P6\n320 200\n255\n", 15);

  for(i=0;i<16000;i+=4) {
    d[3] = mmu_read_word(scraddr+i*2);
    d[2] = mmu_read_word(scraddr+i*2+2);
    d[1] = mmu_read_word(scraddr+i*2+4);
    d[0] = mmu_read_word(scraddr+i*2+6);
    shifter_build_ppm_16pxl(i, d, &image[4*3*i]);
  }
  write(ppmout, image, 192000);
}

void shifter_do_interrupts(struct cpu *cpu)
{
  long tmpcpu;
  tmpcpu = cpu->cycle - lastcpucnt;
  if(tmpcpu < 0) {
    tmpcpu += MAX_CYCLE;
  }

  if((tmpcpu%4) != 0) {
    printf("DEBUG: ERROR!!!! tmpcpu == %ld\n", tmpcpu);
    printf("DEBUG: ERROR!!!! cpu->pc == %08x\n", cpu->pc);
  }
    
  vsync -= tmpcpu;
  if(vsync < 0) {
    /* vsync_interrupt */
    scrptr = curaddr = scraddr;
    shifter_gen_picture(160256);
    //vsync += MAX_CYCLE/((syncreg&2)?50:60);
    vsync += 160256;
    //    printf("DEBUG: vsync %d == %ld\n", vcnt++, 160256-vsync);
    hline = 0;
    hsync = 160;
    shifter_build_image();
    lastrasterpos = 0;
    //    shifter_build_ppm();
    screen_swap();
    if(IPL < 4)
      cpu_set_exception(28);
  }
  hsync -= tmpcpu;
  if(hsync < 0) {
    /* hsync_interrupt */
    if(hline >= (TOPBORDER-1))
      mfp_do_timerb_event(cpu);
    hsync += 512;
    //    gen_scrptr(160256-vsync);
    hline++;
    if(IPL < 2)
      cpu_set_exception(26);
  }
  
  lastcpucnt = cpu->cycle;
}

void shifter_print_status()
{
  int i;

  printf("Pal: ");
  for(i=0;i<15;i++) {
    printf("%04x ", palette[i]);
  }
  printf("\n");
  printf("SCR: 0x%08x\n", scraddr);
  printf("PTR: 0x%08x\n", scrptr);
  printf("Res: %d  Sync: %s\n", resolution, (syncreg&2)?"50Hz":"60Hz");
  printf("\n");
}

int shifter_get_vsync()
{
  return 160256-vsync;
}
