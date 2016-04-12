#include "clock.h"
#include "glue.h"
#include "mmu.h"
#include "shifter.h"

void clock_step(void)
{
  glue_clock();
  mmu_clock();
  shifter_clock();
}
