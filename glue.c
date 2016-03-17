/*
 * Glue		Connects to
 *		practically everything
 */

#include "mmu.h"
#include "glue.h"
#include "diag.h"

#define HBLSIZE 512
#define HBLPRE 64
#define HBLSCR 320
#define HBLPOST 88
#define VBLSIZE 313
#define VBLPRE 64
#define VBLSCR 200
#define VBLPOST 40

#define MODE_50 0
#define MODE_60 1
#define MODE_71 2

static int h, v;
static int mode;
static int line;
static int counter;
static int freq = 1;

HANDLE_DIAGNOSTICS(glue)

void glue_set_resolution(int resolution)
{
  if(resolution == 2)
    mode = MODE_71;
  else if(freq == 0)
    mode = MODE_60;
  else
    mode = MODE_50;
}

static void glue_machine(void)
{
  counter++;
  if(counter & 1)
    return;

  switch(counter) {
  case   4: if(mode == MODE_71) h = 1; break;
  //    30: if(mode != MODE_71) blank off
  case  52: if(mode == MODE_60) h = 1; break;
  //    54: if(mode == MODE_50) PAL
  case  56: if(mode == MODE_50) h = 1; break;
  case 164: if(mode == MODE_71) h = 0; break;
  //   184: if(mode == MODE_71) blank on
  //   224: if(mode == MODE_71) wrap
  case 372: if(mode == MODE_60) h = 0; break;
  case 376: if(mode == MODE_50) h = 0; break;
  //   450: if(mode != MODE_71) blank on
  //   508: if(mode == MODE_60) wrap
  case 512:
    counter = 0;
    line++; 
    switch(line) {
    case  64: v = 1; break;
    case 264: v = 0; break;
    case 313: line = 0; mmu_vsync(); break;
    }
    break;
  }

  if(counter & 3)
    return;
  mmu_de(h && v);
}

void glue_advance(LONG cycles)
{
  int i;

  TRACE("Advance %d cycles", cycles);

  for(i = 0; i < cycles; i++) {
    glue_machine();
  }
}

void glue_reset()
{
  glue_set_resolution(0);
  h = v = 0;
  line = 0;
  counter = 0;
}

void glue_init()
{
  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(glue, "GLUE");
  glue_reset();
}
