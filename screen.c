#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

#include "screen.h"

static int disable = 0;
static SDL_Surface *screen;

#define PADDR(x, y) (screen->pixels + \
                         ((y) + BORDER_SIZE) * screen->pitch + \
                         ((x) + BORDER_SIZE) * screen->format->BytesPerPixel)

void screen_init()
{
  /* should be rewritten with proper error checking */
  
  if(disable) return;
#if 0
  SDL_Init(SDL_INIT_VIDEO);
  atexit(SDL_Quit);
#endif

  screen = SDL_SetVideoMode(640, 400, 24, SDL_HWSURFACE|SDL_DOUBLEBUF);
}

void screen_copyimage(unsigned char *src)
{
  int i;
  for(i=0;i<314;i++) {
    memcpy(PADDR(0, i), src+512*3*i, 512*3);
  }
}

void screen_putpixel(int x, int y, long c)
{
  unsigned char *p;

  if(disable) return;

  p = PADDR(x*2, y*2);
  p[0] = (c>>16)&0xff;
  p[1] = (c>>8)&0xff;
  p[2] = c&0xff;
  p = PADDR(x*2+1, y*2);
  p[0] = (c>>16)&0xff;
  p[1] = (c>>8)&0xff;
  p[2] = c&0xff;
  p = PADDR(x*2, y*2+1);
  p[0] = (c>>16)&0xff;
  p[1] = (c>>8)&0xff;
  p[2] = c&0xff;
  p = PADDR(x*2+1, y*2+1);
  p[0] = (c>>16)&0xff;
  p[1] = (c>>8)&0xff;
  p[2] = c&0xff;
}

void screen_swap()
{
  if(disable) return;
  SDL_Flip(screen);
}

void screen_disable(int yes)
{
  if(yes)
    disable = 1;
  else
    disable = 0;
}

int screen_check_disable()
{
  return disable;
}
