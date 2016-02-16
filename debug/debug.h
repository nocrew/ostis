#ifndef DEBUG_H
#define DEBUG_H

#include "common.h"
#include <SDL.h>

#define KEY_NORMAL 0
#define KEY_EDIT   1

#define DEBUG_QUIT 1
#define DEBUG_CONT 0

#define DEBUG_NORMAL 0
#define DEBUG_INDICATE_RASTERPOS 1

void debug_init();
void debug_scrswap();
int debug_event();
int debug_short_event();
LONG debug_win_addr(int);
int debug_event_parse(SDL_Event);

extern int debug_window_id;

#endif
