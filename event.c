#include <stdio.h>
#include <SDL/SDL.h>
#include "common.h"
#include "event.h"
#include "scancode.h"
#include "ikbd.h"
#include "cpu.h"

static int tstart;
static int tend;

static int event_key(SDL_KeyboardEvent key, int state)
{
  SDL_keysym k;
  Uint16 u;

  k = key.keysym;
  u = k.unicode;

  if((u == ' ') ||
     ((u >= '0') && (u <= '9')) ||
     ((u >= 'a') && (u <= 'z')) ||
     ((u == '\r'))
     ) {
    ikbd_queue_key(scancode[u], state);
  } else if((k.sym >= SDLK_F1) && (k.sym <= SDLK_F10)) {
    ikbd_queue_key(SCAN_F1+k.sym-SDLK_F1, state);
  } else if(k.sym == SDLK_F11) {
    if(state == EVENT_RELEASE) {
#if DEBUG
      return EVENT_DEBUG;
#else
      SDL_Quit();
      exit(0);
#endif
    }
  } else if(k.sym == SDLK_F12) {
    if(state == EVENT_RELEASE) {
      printf("DEBUG: cpu->pc == %08x\n", cpu->pc);
      tend = SDL_GetTicks();
      printf("DEBUG: Speed: %g FPS\n",
	     (shifter_framecnt(0)*1000.0)/(tend-tstart));
    } else {
      shifter_framecnt(-1);
      tstart = SDL_GetTicks();
    }
  } else if(k.sym == SDLK_LALT) {
    ikbd_queue_key(SCAN_ALT, state);
  } else if(k.sym == SDLK_UP) {
    ikbd_queue_key(SCAN_UP, state);
  } else if(k.sym == SDLK_DOWN) {
    ikbd_queue_key(SCAN_DOWN, state);
  } else if(k.sym == SDLK_LEFT) {
    ikbd_queue_key(SCAN_LEFT, state);
  } else if(k.sym == SDLK_RIGHT) {
    ikbd_queue_key(SCAN_RIGHT, state);
  } else if(k.sym == SDLK_END) {
    ikbd_queue_key(SCAN_INSERT, state);
  } else if(k.sym == SDLK_PAGEDOWN) {
    ikbd_queue_key(SCAN_DELETE, state);
  } else {
    printf("Unimplemented key: %d\n", k.sym);
  }

  return EVENT_NONE;
}

void event_init()
{
  SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
  SDL_EnableUNICODE(1);
}

void event_exit()
{
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
}

int event_poll()
{
  SDL_Event ev;

  if(!SDL_PollEvent(&ev)) {
    return EVENT_NONE;
  }

  switch(ev.type) {
  case SDL_KEYDOWN:
    return event_key(ev.key, EVENT_PRESS);
  case SDL_KEYUP:
    return event_key(ev.key, EVENT_RELEASE);
  case SDL_QUIT:
#if DEBUG
    return EVENT_DEBUG;
#else
    SDL_Quit();
    exit(0);
#endif
  }

  return EVENT_NONE;
}
