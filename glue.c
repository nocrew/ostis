/*
 * Glue		Connects to
 *		practically everything
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
static int freq = 1;

HANDLE_DIAGNOSTICS(glue)

void glue_set_resolution(int resolution)
{
  DEBUG("Resolution %d", resolution);
  if(resolution == 2) {
    mode_fn = mode_71;
    counter = 0;
    line = 0;
    h = 0;
    v = 0;
  }
  else if(freq == 0)
    mode_fn = mode_60;
  else
    mode_fn = mode_50;
}

static void mode_50(void)
{
  switch(counter) {
  //    30: blank off
  case  54: end = 512;
  case  56: h = 1; break;
  case 376: h = 0; break;
  //   450: blank on
  default:
    if (counter >= end) {
      TRACE("PAL horizontal retrace");
      counter = 0;
      line++;
      switch(line) {
      case  63: v = 1; break;
      case 263: v = 0; break;
      case 313: line = 0; mmu_vsync(); break;
      }
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
  default:
    if(counter >= end) {
      TRACE("NTSC horizontal retrace");
      counter = 0;
      line++;
      switch(line) {
      case  34: v = 1; break;
      case 234: v = 0; break;
      case 263: line = 0; mmu_vsync(); break;
      }
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
      case 501: line = 0; mmu_vsync(); break;
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
