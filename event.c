#include <stdio.h>
#include <SDL/SDL.h>
#include "common.h"
#include "event.h"
#include "scancode.h"
#include "ikbd.h"
#include "cpu.h"

static int event_key(SDL_KeyboardEvent key, int state)
{
  SDL_keysym k;
  Uint16 u;

  k = key.keysym;
  u = k.unicode;

  if((u == ' ') ||
     ((u >= '0') && (u <= '9')) ||
     ((u >= 'a') && (u <= 'z'))
     ) {
    ikbd_queue_key(scancode[u], state);
  } else if((k.sym >= SDLK_F1) && (k.sym <= SDLK_F10)) {
    ikbd_queue_key(SCAN_F1+k.sym-SDLK_F1, state);
  } else if(k.sym == SDLK_F11) {
    if(state == EVENT_RELEASE)
      return EVENT_DEBUG;
  } else if(k.sym == SDLK_F12) {
    if(state == EVENT_RELEASE)
      printf("DEBUG: cpu->pc == %08x\n", cpu->pc);
  } else {
    printf("Unimplemented key: %d\n", k.sym);
  }

  return EVENT_NONE;
}

void event_init()
{
  SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
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
    return EVENT_DEBUG;
  }

  return EVENT_NONE;
}
