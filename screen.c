#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

#include "screen.h"
#include "debug/display.h"

static int disable = 0;
static SDL_Surface *screen;

#define PADDR(x, y) (screen->pixels + \
                         ((y) + BORDER_SIZE) * screen->pitch + \
                         ((x) + BORDER_SIZE) * screen->format->BytesPerPixel)

void screen_init()
{
  /* should be rewritten with proper error checking */
  Uint32 rmask, gmask, bmask, amask;

  if(disable) return;
#if 0
  SDL_Init(SDL_INIT_VIDEO);
  atexit(SDL_Quit);
#endif
  

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  amask = 0x00000000;
  rmask = 0x00ff0000;
  gmask = 0x0000ff00;
  bmask = 0x000000ff;
#else
  amask = 0x00000000;
  rmask = 0x000000ff;
  gmask = 0x0000ff00;
  bmask = 0x00ff0000;
#endif

  screen = SDL_CreateRGBSurface(SDL_SWSURFACE, 512, 314, 24,
				rmask, gmask, bmask, amask);
#if 0
  screen = SDL_SetVideoMode(640 + BORDER_SIZE * 2,
                         400 + BORDER_SIZE * 2,
                         24, SDL_HWSURFACE|SDL_DOUBLEBUF);
#endif
}

void screen_copyimage(unsigned char *src)
{
#if 0
  int i;
#endif

  //  memcpy(screen->pixels, src, 512*314*3);
#if 0
  for(i=0;i<314;i++) {
    memcpy(PADDR(0, i), src+512*3*i, 512*3);
  }
#endif
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
  SDL_Rect dst;
  if(disable) return;

  dst.x = BORDER_SIZE;
  dst.y = BORDER_SIZE;
  
  SDL_BlitSurface(screen, NULL, display_get_screen(), &dst);
  SDL_Flip(display_get_screen());
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

void *screen_pixels()
{
  return screen->pixels;
}
