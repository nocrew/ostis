#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "font.h"
#include "win.h"

static SDL_Surface *scr;
static SDL_Surface *font[4][256];

#define PADDR(f, x, y) (f->pixels + (y)*f->pitch + (x)*f->format->BytesPerPixel)

void display_clear_screen()
{
  int i;
  char *p;

  p = scr->pixels;

  for(i=0;i<(scr->w*scr->format->BytesPerPixel*scr->h);i++) {
    p[i] = 0xff;
  }
}

void display_put_pixel(SDL_Surface *s, int x, int y, long c)
{
  char *p;

  p = PADDR(s, x, y);
  p[0] = (c>>16)&0xff;
  p[1] = (c>>8)&0xff;
  p[2] = (c&0xff);
}

void display_put_pixel_screen(int x, int y, long c)
{
  char *p;

  p = PADDR(scr, x + BORDER_SIZE, y + BORDER_SIZE);
  p[0] = (c>>16)&0xff;
  p[1] = (c>>8)&0xff;
  p[2] = (c&0xff);
}

static void build_char(int c[], SDL_Surface *f, int h, int inv)
{
  int x,y,b;

  for(y=0; y<h; y++) {
    for(x=0;x<8;x++) {
      b = c[y]&(1<<(7-x));
      if(inv)
	display_put_pixel(f, x, y, b?0xffffff:0);
      else
	display_put_pixel(f, x, y, b?0:0xffffff);
    }
  }
}

static void build_font()
{
  int i;
  Uint32 rmask, gmask, bmask, amask;

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


  for(i=0;i<256;i++) {
    font[0][i] = SDL_CreateRGBSurface(SDL_HWSURFACE, 8, 8, 24,
				      rmask, gmask, bmask, amask);
    font[2][i] = SDL_CreateRGBSurface(SDL_HWSURFACE, 8, 8, 24,
				      rmask, gmask, bmask, amask);
    build_char(font8x8[i], font[0][i], 8, 0);
    build_char(font8x8[i], font[2][i], 8, 1);
  }
  for(i=0;i<256;i++) {
    font[1][i] = SDL_CreateRGBSurface(SDL_HWSURFACE, 8, 16, 24,
				      rmask, gmask, bmask, amask);
    font[3][i] = SDL_CreateRGBSurface(SDL_HWSURFACE, 8, 16, 24,
				      rmask, gmask, bmask, amask);
    build_char(font8x16[i], font[1][i], 16, 0);
    build_char(font8x16[i], font[3][i], 16, 1);
  }
}

void display_put_char(int x, int y, int f, unsigned char c)
{
  SDL_Rect dst;

  dst.x = x + BORDER_SIZE;
  dst.y = y + BORDER_SIZE;
  
  SDL_BlitSurface(font[f][c], NULL, scr, &dst);
}

void display_setup()
{
  if(SDL_Init(SDL_INIT_VIDEO) == -1) {
    fprintf(stderr, "Unable to initialize SDL.\n");
    exit(-2);
  }

  atexit(SDL_Quit);

  scr = SDL_SetVideoMode(640 + BORDER_SIZE * 2,
                         400 + BORDER_SIZE * 2,
                         24, SDL_HWSURFACE|SDL_DOUBLEBUF);

  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
  SDL_EnableUNICODE(1);

  build_font();
}

void display_swap_screen()
{
  win_draw_screen();
  SDL_Flip(scr);
}

SDL_Surface *display_get_screen()
{
  return scr;
}
