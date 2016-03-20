#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <SDL_image.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>

#include "screen.h"
#include "shifter.h"
#include "debug/display.h"
#include "diag.h"

struct monitor {
  int width, height, columns, lines;
  int crop_offset_x, crop_offset_y;
  int crop_width, crop_height;
};

static int disable = 0;
static SDL_Surface *screen;
static SDL_Texture *texture = NULL;
static SDL_Window *window;
static SDL_Renderer *renderer;
int screen_window_id;
static SDL_Texture *rasterpos_indicator[2];
static int rasterpos_indicator_cnt = 0;
static int screen_grabbed = 0;
static int screen_fullscreen = 0;
static char *rasterpos;
static char *next_line;
static char *rgbimage;

static int ppm_fd;
static int framecnt;
static int64_t last_framecnt_usec;
static int usecs_per_framecnt_interval = 1;
int64_t last_vsync_ticks = 0;

struct monitor monitors[] = {
  { 1024, 626, 1024, 313, 64, 33, 768, 560 },
  { 896, 501, 896, 501, 0, 0, 896, 501 }
};

struct monitor mon;

#define PADDR(x, y) (screen->pixels + \
                         ((y) + BORDER_SIZE) * screen->pitch + \
                         ((x) + BORDER_SIZE) * screen->format->BytesPerPixel)

HANDLE_DIAGNOSTICS(screen)

void screen_make_texture(const char *scale)
{
  static const char *old_scale = "";
  int pixelformat = SDL_PIXELFORMAT_RGB24;

  if (monitor_sm124)
    mon = monitors[1];
  else
    mon = monitors[0];

  if(strcmp(scale, old_scale) == 0)
    return;

  if(texture != NULL)
    SDL_DestroyTexture(texture);

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, scale);
  texture = SDL_CreateTexture(renderer,
			      pixelformat,
			      SDL_TEXTUREACCESS_STREAMING,
			      mon.columns, mon.lines);
}

SDL_Texture *screen_generate_rasterpos_indicator(int color)
{
  Uint32 rmask, gmask, bmask, amask;
  SDL_Surface *rscreen;
  SDL_Texture *rtext;
  int i;
  char *p;
  
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

  rscreen = SDL_CreateRGBSurface(0, 4, 1, 24, rmask, gmask, bmask, amask);

  p = rscreen->pixels;
  
  for(i=0;i<rscreen->w;i++) {
    p[i*rscreen->format->BytesPerPixel+0] = (color<<16)&0xff;
    p[i*rscreen->format->BytesPerPixel+1] = (color<<8)&0xff;
    p[i*rscreen->format->BytesPerPixel+2] = color&0xff;
  }

  rtext = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24,
                            SDL_TEXTUREACCESS_STREAMING,
                            4, 1);
  
  SDL_UpdateTexture(rtext, NULL, rscreen->pixels, rscreen->pitch);
  
  return rtext;
}

void screen_init()
{
  /* should be rewritten with proper error checking */
  Uint32 rmask, gmask, bmask, amask;
  SDL_Surface *icon;
  
  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(screen, "SCRN");

  if(disable) return;
  
#if SDL_BYTEORDER != SDL_BIG_ENDIAN
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

  if (monitor_sm124)
    mon = monitors[1];
  else
    mon = monitors[0];

  if (crop_screen) {
    mon.width = mon.crop_width;
    mon.height = mon.crop_height;
  }
  
    window = SDL_CreateWindow("Main screen",
			      SDL_WINDOWPOS_UNDEFINED,
			      SDL_WINDOWPOS_UNDEFINED,
			      mon.width,
			      mon.height,
			      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    screen = SDL_CreateRGBSurface(0, mon.columns, mon.lines,
				  24, rmask, gmask, bmask, amask);
    renderer = SDL_CreateRenderer(window, -1, 0);
    screen_window_id = SDL_GetWindowID(window);
    DEBUG("screen_window_id == %d", screen_window_id);
    screen_make_texture(SDL_SCALING_NEAREST);

  if(screen == NULL) {
    FATAL("Did not get a video mode");
  }

  icon = IMG_Load("logo-main.png");
  SDL_SetWindowIcon(window, icon);
  
  if(ppmoutput) {
    ppm_fd = open("ostis.ppm", O_WRONLY|O_CREAT|O_TRUNC, 0644);
  }

  rgbimage = screen->pixels;
  rasterpos_indicator[0] = screen_generate_rasterpos_indicator(0xffffff);
  rasterpos_indicator[1] = screen_generate_rasterpos_indicator(0xff0000);

  framecnt = 0;

  screen_vsync();

  if(debugger) {
    debug_raise_window();
  }
}

float screen_fps()
{
  if(usecs_per_framecnt_interval) {
    return 1000000*64.0/usecs_per_framecnt_interval;
  } else {
    return 0;
  }
}

int screen_framecnt(int c)
{
  if(c == -1) {
    framecnt = 0;
  }
  return framecnt;
}

static void screen_build_ppm()
{
  int x,y,c;
  char header[80];
  unsigned char frame[384*288*3];

  c = 0;

  for(y=0;y<288;y++) {
    for(x=0;x<384;x++) {
      frame[c*3+0] = rgbimage[((y+12)*512+x+32)*6+2];
      frame[c*3+1] = rgbimage[((y+12)*512+x+32)*6+1];
      frame[c*3+2] = rgbimage[((y+12)*512+x+32)*6+0];
      c++;
    }
  }

  sprintf(header, "P6\n%d %d\n255\n", 384, 288);
  if(write(ppm_fd, header, strlen(header)) != strlen(header))
    WARNING(write);
  if (write(ppm_fd, frame, 384*288*3) != 384*288*3)
    WARNING(write);
}

void screen_copyimage(unsigned char *src)
{
}

void screen_clear()
{
}

int screen_get_vsync()
{
  return (rasterpos - rgbimage) / 6;
}

void screen_swap(int indicate_rasterpos)
{
  SDL_Rect dst,src;
  
  if(disable) return;

    if(debugger) {
      display_swap_screen();
    }
    SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);
    if(crop_screen) {
      src.x = mon.crop_offset_x;
      src.y = mon.crop_offset_y;
      src.w = mon.crop_width;
      src.h = mon.crop_height;
      SDL_RenderCopy(renderer, texture, &src, NULL);
    } else {
      SDL_RenderCopy(renderer, texture, NULL, NULL);
    }
    if(indicate_rasterpos) {
      int rasterpos = screen_get_vsync();
      dst.x = 2*(rasterpos%512)-8;
      dst.y = 2*(rasterpos/512);
      dst.w = 8;
      dst.h = 2;
      SDL_RenderCopy(renderer, rasterpos_indicator[rasterpos_indicator_cnt&1], NULL, &dst);
      rasterpos_indicator_cnt++;
    }
    SDL_RenderPresent(renderer);
}

void screen_disable(int yes)
{
  if(yes) {
    disable = 1;
  } else {
    disable = 0;
  }
}

int screen_check_disable()
{
  return disable;
}

static void set_screen_grabbed(int grabbed)
{
  int w, h;

  if(grabbed) {
    SDL_SetWindowGrab(window, SDL_TRUE);
    SDL_GetWindowSize(window, &w, &h);
    SDL_WarpMouseInWindow(window, w/2, h/2);
    SDL_ShowCursor(0);
    screen_grabbed = 1;
  } else {
    SDL_SetWindowGrab(window, SDL_FALSE);
    SDL_ShowCursor(1);
    screen_grabbed = 0;
  }
}

void screen_toggle_grab()
{
  set_screen_grabbed(!screen_grabbed);
}

void screen_toggle_fullscreen()
{
  screen_fullscreen = !screen_fullscreen;
  SDL_SetWindowFullscreen(window, screen_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  set_screen_grabbed(screen_fullscreen);
}

void screen_draw(int r, int g, int b)
{
  *rasterpos++ = r;
  *rasterpos++ = g;
  *rasterpos++ = b;
}

static int64_t usec_count() {
  static struct timeval tv;

  gettimeofday(&tv,NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

void screen_vsync(void)
{
  int64_t current_ticks = 0;
  int64_t remaining = 0;

  TRACE("Vsync");

  if(ppmoutput) {
    screen_build_ppm();
  }
  if(vsync_delay) {
    current_ticks = usec_count();
    remaining = current_ticks - last_vsync_ticks;
    while(remaining < 20000) {
      current_ticks = usec_count();
      remaining = current_ticks - last_vsync_ticks;
    }
    last_vsync_ticks = current_ticks;
  }
  screen_swap(SCREEN_NORMAL);

  framecnt++;
  if((framecnt&0x3f) == 0) {
    current_ticks = usec_count();
    usecs_per_framecnt_interval = current_ticks - last_framecnt_usec;
    last_framecnt_usec = current_ticks;
  }

  rasterpos = rgbimage;
  next_line = rgbimage + screen->pitch;
}

void screen_hsync(void)
{
  rasterpos = next_line;
  next_line += screen->pitch;
}
