#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "font.h"
#include "win.h"

static SDL_Window *debug_window;
static SDL_Surface *debug_screen;
static SDL_Surface *font[4][256];
static SDL_Texture *debug_texture;
static SDL_Renderer *debug_renderer;
int debug_window_id = 0;

#define PADDR(f, x, y) (f->pixels + (y)*f->pitch + (x)*f->format->BytesPerPixel)

void display_clear_screen()
{
  int i;
  char *p;

  p = debug_screen->pixels;

  for(i=0;i<(debug_screen->w*debug_screen->format->BytesPerPixel*debug_screen->h);i++) {
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

  p = PADDR(debug_screen, 2*x + BORDER_SIZE, y + BORDER_SIZE);
  p[0] = (c>>16)&0xff;
  p[1] = (c>>8)&0xff;
  p[2] = (c&0xff);
  p[3] = (c>>16)&0xff;
  p[4] = (c>>8)&0xff;
  p[5] = (c&0xff);
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
    font[0][i] = SDL_CreateRGBSurface(0, 8, 8, 24,
				      rmask, gmask, bmask, amask);
    font[2][i] = SDL_CreateRGBSurface(0, 8, 8, 24,
				      rmask, gmask, bmask, amask);
    build_char(font8x8[i], font[0][i], 8, 0);
    build_char(font8x8[i], font[2][i], 8, 1);
  }
  for(i=0;i<256;i++) {
    font[1][i] = SDL_CreateRGBSurface(0, 8, 16, 24,
				      rmask, gmask, bmask, amask);
    font[3][i] = SDL_CreateRGBSurface(0, 8, 16, 24,
				      rmask, gmask, bmask, amask);
    build_char(font8x16[i], font[1][i], 16, 0);
    build_char(font8x16[i], font[3][i], 16, 1);
  }
}

void display_put_char(int x, int y, int f, unsigned char c)
{
  SDL_Rect dst;

  dst.x = 2*x + BORDER_SIZE;
  dst.y = y + BORDER_SIZE;
  dst.w = 16;
  dst.h = 8*(f&1)+8;
  
  SDL_BlitScaled(font[f][c], NULL, debug_screen, &dst);
}

void display_render_screen()
{
  SDL_UpdateTexture(debug_texture, NULL, debug_screen->pixels, debug_screen->pitch);
  SDL_RenderCopy(debug_renderer, debug_texture, NULL, NULL);
  SDL_RenderPresent(debug_renderer);
}

void display_swap_screen()
{
  win_draw_screen();
  display_render_screen();
}

void display_setup()
{
  Uint32 rmask, gmask, bmask, amask;
  
  if(SDL_Init(SDL_INIT_VIDEO) == -1) {
    fprintf(stderr, "Unable to initialize SDL.\n");
    exit(-2);
  }

  atexit(SDL_Quit);

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, SDL_SCALING_LINEAR);
  debug_window = SDL_CreateWindow("Debugger screen", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  (640 + BORDER_SIZE * 2),
                                  (400 + BORDER_SIZE * 2),
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

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

  debug_screen = SDL_CreateRGBSurface(0,
                                      2*(640 + BORDER_SIZE * 2),
                                      (400 + BORDER_SIZE * 2),
                                      24,
                                      rmask, gmask, bmask, amask);
  debug_renderer = SDL_CreateRenderer(debug_window, -1, 0);
  debug_texture = SDL_CreateTexture(debug_renderer,
                                    SDL_PIXELFORMAT_RGB24,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    2*(640 + BORDER_SIZE * 2),
                                    (400 + BORDER_SIZE * 2));

  debug_window_id = SDL_GetWindowID(debug_window);

  display_swap_screen();
  SDL_ShowWindow(debug_window);
  
  build_font();
}

SDL_Surface *display_get_screen()
{
  return debug_screen;
}
