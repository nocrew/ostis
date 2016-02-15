#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

#include "screen.h"
#include "shifter.h"
#include "debug/display.h"

static int disable = 0;
static SDL_Surface *screen;
static SDL_Texture *texture = NULL;
static SDL_Window *window;
static SDL_Renderer *renderer;

#define PADDR(x, y) (screen->pixels + \
                         ((y) + BORDER_SIZE) * screen->pitch + \
                         ((x) + BORDER_SIZE) * screen->format->BytesPerPixel)

void screen_make_texture(const char *scale)
{
  static const char *old_scale = "";

  if(strcmp(scale, old_scale) == 0)
    return;

  if(texture != NULL)
    SDL_DestroyTexture(texture);

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, scale);
  texture = SDL_CreateTexture(renderer,
			      SDL_PIXELFORMAT_BGR24,
			      SDL_TEXTUREACCESS_STREAMING,
			      2*512, 314);
}

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

  if(debugger) {
    screen = SDL_CreateRGBSurface(0, 2*512, 314, 24,
    				  rmask, gmask, bmask, amask);
  } else {
    window = SDL_CreateWindow("Main screen", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 628, SDL_WINDOW_RESIZABLE);
    screen = SDL_CreateRGBSurface(0, 2*512, 314, 24,
    				  rmask, gmask, bmask, amask);
    renderer = SDL_CreateRenderer(window, -1, 0);
    screen_make_texture(SDL_SCALING_NEAREST);
  }

  if(screen == NULL) {
    fprintf(stderr, "Did not get a video mode\n");
    exit(1);
  }
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

void screen_clear()
{
  int i;
  unsigned char *p;

  p = screen->pixels;

  for(i=0;i<512*314;i++) {
    if((((i/512)&1) && (i&1)) ||
       (!((i/512)&1) && !(i&1)))
      p[i*3+0] = p[i*3+1] = p[i*3+2] = 0;
    else {
      if(shifter_on_display(i)) {
	p[i*3+0] = p[i*3+1] = p[i*3+2] = 0xf0;
      } else {
	p[i*3+0] = p[i*3+1] = p[i*3+2] = 0xff;
      }
    }
  }
}

void screen_swap()
{
  SDL_Rect dst;

  if(disable) return;

  if(debugger) {
    SDL_Surface *debug_display;
    dst.x = BORDER_SIZE;
    dst.y = BORDER_SIZE;

    debug_display = display_get_screen();
    
    SDL_BlitSurface(screen, NULL, debug_display, &dst);
    display_render_screen();
    //    SDL_Flip(display_get_screen());
  } else {
    SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    //    SDL_Flip(screen);
  }
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


