#include "ucode.h"

static int ucycles;
static uop_t *upc;

void ujump(uop_t *uop, int cyles)
{
  upc = uop;
  ucycles = cyles;
}

int ustep(void)
{
  if(ucycles == 0)
    return 1;

  ucycles--;
  (*upc)();
  upc++;
  
  return 0;
}
