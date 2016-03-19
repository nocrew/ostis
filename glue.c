/*
 * Glue		Connects to
 *
 * [CONTROL]
 *  CLK		8 MHz
 *  RESET	RESET
 *  2MHZ	2 Mhz
 *  500KHZ	500 kHz
 * [BUS]
 *  D0-D1	BUS:D0-D1
 *  FC0-FC1
 *  AS
 *  UDS
 *  LDS
 *  DTACK
 *  BERR
 *  BR
 *  BGI
 *  BGO
 *  BGACK
 * [INTERRUPT]
 *  IACK	MFP:IACK
 *  IPL1	CPU:IPL1
 *  IPL2	CPU:IPL2
 *  MFPINT	MFP:IRQ
 * [CHIP SELECT]
 *  6850CS	2x ACIA:CS0
 *  MFPCS	MFP:CS
 *  SNDCS	PSG:BC1, PSG:BDIR
 *  ROM0-5	ROM
 * [VIDEO]
 *  DE		MMU:DE, SHIFTER:DE, MFP:TBI
 *  VSYNC	MMU:VSYNC, SCREEN:VSYNC
 *  HSYNC	SCREEN:HSYNC
 *  BLANK	SHIFTER:RGB
 * [?]
 *  RAM		MMU:RAM
 *  DEV		MMU:DEV
 *  DMA		MMU:DMA
 *  VMA		CPU:VMA
 *  VPA		CPU:VPA
 *  FCS		?
 *  RDY		?
 */

#include "mmu.h"
#include "glue.h"
#include "diag.h"

typedef void modefn_t(void);
static modefn_t mode_50, mode_60, mode_71;
static modefn_t *mode_fn;

static int h, v;
static int end;
static int line;
static int counter;
static int res = 0;
static int freq = 2;

HANDLE_DIAGNOSTICS(glue)

static void set_mode(void)
{
  if(res == 2) {
    mode_fn = mode_71;
    counter = 0;
    line = 0;
    h = 0;
    v = 0;
  } else if(freq == 0)
    mode_fn = mode_60;
  else
    mode_fn = mode_50;
}

void glue_set_resolution(int resolution)
{
  DEBUG("Resolution %d", resolution);
  res = resolution;
  set_mode();
}

void glue_set_sync(int sync)
{
  DEBUG("S%d @ %03d;%03d", sync, line, counter);
  freq = sync;
  set_mode();
}

static void vsync(void)
{
  DEBUG("Vsync");
  line = 0;
  v = 0;
  mmu_vsync();
  screen_vsync();
}

static void mode_50(void)
{
  switch(counter) {
  //    30: blank off
  case  54: end = 512;
  case  56: h = 1; break;
  case 376: h = 0; break;
  //   450: blank on
  case 502:
    switch(line) {
    case  63: v = 1; break;
    case 263: v = 0; break;
    }
    break;
  default:
    if (counter == end) {
      TRACE("PAL horizontal retrace");
      counter = 0;
      line++;
      if(line == 313)
	vsync();
    }
    break;
  }
}

void mode_60(void)
{
  switch(counter) {
  //    30: blank off
  case  52: h = 1; break;
  case  54: end = 508; break;
  case 372: h = 0; break;
  //   450: blank on
  case 502:
    switch(line) {
    case  34: v = 1; break;
    case 234: v = 0; break;
    }
    break;
  default:
    if(counter == end) {
      TRACE("NTSC horizontal retrace");
      counter = 0;
      line++;
      if(line == 263)
	vsync();
    }
    break;
  }
}

void mode_71(void)
{
  switch(counter) {
  case   4: h = 1; break;
  case 164: h = 0; break;
  //   184: blank on
  default:
    if(counter >= 224) {
      TRACE("Monochrome horizontal retrace");
      counter = 0;
      line++;
      switch(line) {
      case  50: v = 1; break;
      case 450: v = 0; break;
      case 501: vsync(); break;
      }
    }
  }
}

static void glue_machine(void)
{
  if(counter & 1)
    return;
  mode_fn();
  if(counter & 3)
    return;
  mmu_de(h && v);
}

void glue_advance(LONG cycles)
{
  int i;
  for(i = 0; i < cycles; i++) {
    glue_machine();
    counter++;
    ASSERT(counter <= 512);
  }
}

void glue_reset()
{
  glue_set_resolution(0);
  h = v = 0;
  line = 0;
  counter = 0;
  end = 1000;
}

void glue_init()
{
  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(glue, "GLUE");
  glue_reset();
}
