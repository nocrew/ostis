/*
 * Glue		Connects to
 *
 * [CONTROL]
 *  CLK		8 MHz
 *  RESET	RESET
 *  2MHZ	2 Mhz
 *  500KHZ	500 kHz
 * [BUS]
 *  A2-A23	BUS:A1-A23
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
#include "mfp.h"
#include "glue.h"
#include "diag.h"

typedef void modefn_t(void);
static modefn_t mode_50, mode_60, mode_71;
static modefn_t *mode_fn;

static int h, v;
static int counter_end;
static int line_end;
static int line;
static int counter;
static int res = 0;
static int freq = 2;

HANDLE_DIAGNOSTICS(glue)

static void set_mode(void)
{
  if(res == 2)
    mode_fn = mode_71;
  else if(freq == 0)
    mode_fn = mode_60;
  else
    mode_fn = mode_50;
}

void glue_set_resolution(int resolution)
{
  TRACE("R%d @ %03d;%03d", resolution, line, counter);
  res = resolution;
  set_mode();
}

void glue_set_sync(int sync)
{
  TRACE("S%d @ %03d;%03d", sync, line, counter);
  freq = sync;
  set_mode();
}

static void vsync(void)
{
  TRACE("Vsync");
  line = 0;
  v = 0;
  mmu_vsync();
  screen_vsync();
}

static void mode_50(void)
{
  switch(counter) {
  case  30: shifter_blank(0); break;
  case  54: counter_end = 512; break;
  case  56: h = 1; cpu_ipl1(); break;
  case 256: if(line == 312) cpu_ipl2(); break;
  case 376: h = 0; break;
  case 400: if(v) mfp_do_timerb_event(cpu); break; //Should be 376
  case 450: shifter_blank(1); break;
  case 502:
    switch(line) {
    case   0: line_end = 313; break;
    case  63: v = 1; break;
    case 263: v = 0; break;
    }
    break;
  default:
    if (counter == counter_end) {
      CLOCK("PAL horizontal retrace");
      screen_hsync();
      counter = 0;
      line++;
      if(line == line_end)
	vsync();
    }
    break;
  }
}

void mode_60(void)
{
  switch(counter) {
  case  30: shifter_blank(0); break;
  case  52: h = 1; break;
  case  54: counter_end = 508; break;
  case  56: cpu_ipl1(); break;
  case 256: if(line == 263) cpu_ipl2(); break;
  case 372: h = 0; break;
  case 400: if(v) mfp_do_timerb_event(cpu); break; //Should be 372
  case 450: shifter_blank(1); break;
  case 502:
    switch(line) {
    case   0: line_end = 263; break;
    case  34: v = 1; break;
    case 234: v = 0; break;
    }
    break;
  default:
    if(counter == counter_end) {
      CLOCK("NTSC horizontal retrace");
      screen_hsync();
      counter = 0;
      line++;
      if(line == line_end)
	vsync();
    }
    break;
  }
}

void mode_71(void)
{
  switch(counter) {
  case   4: h = 1; break;
  case  54: counter_end = 224; break;
  case  56: cpu_ipl1(); break;
  case  80: if(line == 500) cpu_ipl2(); break;
  case 164: h = 0; if(v) mfp_do_timerb_event(cpu); break;
  //   184: blank on
  case 220:
    switch(line) {
    case   0: line_end = 501; break;
    case  50: v = 1; break;
    case 450: v = 0; break;
    }
    break;
  default:
    if(counter == counter_end) {
      CLOCK("Monochrome horizontal retrace");
      screen_hsync();
      counter = 0;
      line++;
      if(line == line_end)
	vsync();
    }
    break;
  }
}

void glue_clock(void)
{
  if((counter & 1) == 0) {
    mode_fn();
    mmu_de(h && v);
    shifter_de(h && v);
  }
  counter++;
  ASSERT(counter <= 512);
  ASSERT(line <= line_end);
}

void glue_reset()
{
  glue_set_resolution(0);
  h = v = 0;
  line = 0;
  line_end = 1000;
  counter = 0; // Here be wakestate.
  counter_end = 1000;
}

void glue_init()
{
  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(glue, "GLUE");
  glue_reset();
}
