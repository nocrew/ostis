#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "cpu.h"
#include "mfp.h"
#include "screen.h"
#include "mmu.h"

#define SHIFTERSIZE 256
#define SHIFTERBASE 0xff8200

static WORD palette[16];
static LONG scraddr;
static LONG scrptr;
static BYTE syncreg;
static BYTE resolution;

static long lastcpucnt;
static long vsync;
static long hsync;
static long hline;

static BYTE shifter_read_byte(LONG addr)
{
  switch(addr) {
  case 0xff8201:
    return (scraddr&0xff0000)>>16;
  case 0xff8203:
    return (scraddr&0xff00)>>8;
  case 0xff8205:
    return (scrptr&0xff0000)>>16;
  case 0xff8207:
    return (scrptr&0xff00)>>8;
  case 0xff8209:
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
       (addr <= 0xff825e)) {
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
  shifter_write_byte(addr, (data&0xff00)>>8);
  shifter_write_byte(addr+1, (data&0xff));
}

static void shifter_write_long(LONG addr, LONG data)
{
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
}

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

static void build_16pxl(int wcnt, WORD d[], int dbg)
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
    screen_putpixel(x+(15-i), y, build_color(c), dbg);
  }
}

void shifter_build_image(int dbg)
{
  int i;
  WORD d[4];

  for(i=0;i<16000;i+=4) {
    d[3] = mmu_read_word(scraddr+i*2);
    d[2] = mmu_read_word(scraddr+i*2+2);
    d[1] = mmu_read_word(scraddr+i*2+4);
    d[0] = mmu_read_word(scraddr+i*2+6);
    build_16pxl(i, d, dbg);
  }
}

void shifter_do_interrupts(struct cpu *cpu)
{
  long tmpcpu;
  tmpcpu = cpu->cycle - lastcpucnt;
  if(tmpcpu < 0) {
    tmpcpu += MAX_CYCLE;
  }

  vsync -= tmpcpu;
  if(vsync < 0) {
    /* vsync_interrupt */
    vsync += MAX_CYCLE/((syncreg&2)?50:60);
    hline = 0;
    scrptr = scraddr;
    shifter_build_image(0);
    screen_swap();
    if(IPL < 4)
      cpu_set_exception(28);
  }
  hsync -= tmpcpu;
  if(hsync < 0) {
    /* hsync_interrupt */
    if(hline >= 29)
      mfp_do_timerb_event(cpu);
    hsync += 512;
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
