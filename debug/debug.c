#include <SDL.h>
#include "common.h"
#include "win.h"
#include "edit.h"
#include "display.h"
#include "shifter.h"
#include "debug.h"
#include "event.h"
#include "cpu.h"
#include "cprint.h"
#include "mfp.h"
#include "ikbd.h"

#define VIEW_DISPLAY 0
#define VIEW_DEBUG   1

static int keymode = KEY_NORMAL;
static int debugmode = 0;
static int viewmode = VIEW_DEBUG;

void debug_init()
{
  display_setup();
  win_setup_default();
  display_swap_screen();
}

static void debug_toggle_breakpoint(int wnum)
{
  if(cpu_unset_breakpoint(win[wnum].addr)) {
    cpu_set_breakpoint(win[wnum].addr, 1);
  }
}

static void debug_breakpoint_next_instr()
{
  struct cprint *cprint;
  
  cprint = cprint_instr(cpu->pc);
  cpu_set_breakpoint(cpu->pc + cprint->size, 1);
  free(cprint);
}

static void debug_skip_next_instr()
{
  struct cprint *cprint;
  
  cprint = cprint_instr(cpu->pc);
  cpu->pc += cprint->size;
  free(cprint);
  win_set_message("Skip");
}

static void clear_event_queue()
{
  SDL_Event ev;
  
  while(SDL_PollEvent(&ev));
}

static void debug_set_editmode()
{
  edit_setup();
  keymode = KEY_EDIT;
  clear_event_queue();
}

static int debug_do_key_normal(SDL_KeyboardEvent key)
{
  SDL_Keysym k;
  int ret;

  k = key.keysym;

  win_set_message("");

  switch(k.sym) {
  case SDLK_ESCAPE:
    win[2].addr = cpu->pc;
    break;
  case SDLK_TAB:
    win_cycle_selected();
    break;
  case SDLK_UP:
    win_move_window(MOVE_UP, k.mod&KMOD_SHIFT);
    break;
  case SDLK_DOWN:
    win_move_window(MOVE_DOWN, k.mod&KMOD_SHIFT);
    break;
  case SDLK_LEFT:
    win_move_window(MOVE_LEFT, k.mod&KMOD_SHIFT);
    break;
  case SDLK_RIGHT:
    win_move_window(MOVE_RIGHT, k.mod&KMOD_SHIFT);
    break;
  case SDLK_d:
    if(debugmode) debugmode = 0; else debugmode = 1;
    break;
  case SDLK_t:
    if(k.mod & KMOD_ALT)
      win_toggle_window_type(WIN_CURRENT);
    break;
  case SDLK_f:
    if(k.mod & KMOD_ALT)
      win_toggle_window_font(WIN_CURRENT);
    break;
  case SDLK_s:
    if(k.mod & KMOD_ALT)
      win_toggle_window_split(WIN_CURRENT);
    if(k.mod & KMOD_CTRL)
      debug_skip_next_instr();
    break;
  case SDLK_y:
    if(k.mod & KMOD_ALT)
      win_toggle_window_full(WIN_CURRENT);
    break;
  case SDLK_z:
    if(k.mod & KMOD_CTRL) {
      if((win_get_selected() != 2) && (win_get_selected() != 4))
	win_set_selected(2);
      ret = cpu_step_instr(CPU_TRACE);
      if(debugmode) {
	cpu_print_status(CPU_USE_CURRENT_PC);
	mfp_print_status();
      }
      if(ret == CPU_BREAKPOINT) {
	win_set_message("Breakpoint");
      } else if(ret == CPU_WATCHPOINT) {
	win_set_message("Watchpoint");
      } else {
	win_set_message("Trace");
      }
      win_move_window_to_pc();
      shifter_force_gen_picture();
    }
    break;
  case SDLK_a:
    if(k.mod & KMOD_CTRL) {
      debug_breakpoint_next_instr();
      if((win_get_selected() != 2) && (win_get_selected() != 4))
	win_set_selected(2);
      debug_update_win = 0;
      ret = cpu_run(CPU_DEBUG_RUN);
      debug_update_win = 1;
      if(ret == CPU_BREAKPOINT) {
	win_set_message("Breakpoint");
      } else if(ret == CPU_WATCHPOINT) {
	win_set_message("Watchpoint");
      }
      win_move_window_to_pc();
      shifter_force_gen_picture();
    }
    break;
  case SDLK_r:
    if(k.mod & KMOD_CTRL) {
      if((win_get_selected() != 2) && (win_get_selected() != 4))
	win_set_selected(2);
      debug_update_win = 0;
      ret = cpu_run(CPU_DEBUG_RUN);
      debug_update_win = 1;
      if(ret == CPU_BREAKPOINT) {
	win_set_message("Breakpoint");
      } else if(ret == CPU_WATCHPOINT) {
	win_set_message("Watchpoint");
      }
      win_move_window_to_pc();
    } else if(k.mod & KMOD_ALT) {
      win_set_exwin(EDIT_SETREG);
      debug_set_editmode();
    }
    break;
  case SDLK_m:
    if(win_get_selected() > 1) {
      win_set_exwin(EDIT_SETADDR);
      debug_set_editmode();
    }
    break; 
  case SDLK_v:
    if(viewmode == VIEW_DISPLAY)
      viewmode = VIEW_DEBUG;
    else
      viewmode = VIEW_DISPLAY;
    break;
  case SDLK_l:
    if(k.mod & KMOD_SHIFT) {
      win_set_exwin(EDIT_LABELCMD);
    } else {
      win_set_exwin(EDIT_LABEL);
    }
    debug_set_editmode();
    break;
  case SDLK_b:
    if(k.mod & KMOD_ALT) {
      win_set_exwin(EDIT_SETBRK);
      debug_set_editmode();
    } else if(k.mod & KMOD_CTRL) {
      if(win[win_get_selected()].type == TYPE_DIS)
	debug_toggle_breakpoint(win_get_selected());
    } else {
      cpu_print_breakpoints();
    }
    break;
  case SDLK_w:
    if(k.mod & KMOD_ALT) {
      win_set_exwin(EDIT_SETWATCH);
      debug_set_editmode();
    }
    break;
  case SDLK_c:
    if(k.mod & KMOD_CTRL) return 1;
    screen_clear();
    break;
  case SDLK_F12:
    printf("-------------------------------------------\n");
    cpu_print_status(CPU_USE_CURRENT_PC);
    printf("- - - - - - - - - - - - - - - - - - - - - -\n");
    mfp_print_status();
    printf("- - - - - - - - - - - - - - - - - - - - - -\n");
    ikbd_print_status();
    printf("-------------------------------------------\n");
  default:
    break;
  }
  shifter_build_image(0);
  screen_swap(DEBUG_INDICATE_RASTERPOS);
  display_swap_screen();
  return 0;
}

static int debug_do_text_edit(SDL_TextInputEvent tev)
{
  if(((tev.text[0] >= '0') && (tev.text[0] <= '9')) ||
     ((tev.text[0] >= 'a') && (tev.text[0] <= 'z')) ||
     ((tev.text[0] >= 'A') && (tev.text[0] <= 'Z')) ||
     (tev.text[0] == '[') || (tev.text[0] == ']') ||
     (tev.text[0] == '(') || (tev.text[0] == ')') ||
     (tev.text[0] == '$') || (tev.text[0] == '-') ||
     (tev.text[0] == '+') || (tev.text[0] == '*') ||
     (tev.text[0] == '/') || (tev.text[0] == '_') ||
     (tev.text[0] == '|') || (tev.text[0] == '&') ||
     (tev.text[0] == '^') || (tev.text[0] == '~') ||
     (tev.text[0] == '\\') || (tev.text[0] == '.') ||
     (tev.text[0] == '!') || (tev.text[0] == '<') ||
     (tev.text[0] == '>') ||
     (tev.text[0] == '=') || (tev.text[0] == ',')) {
    edit_insert_char(tev.text[0]);
  }

  display_swap_screen();
  return 0;
}

static int debug_do_key_edit(SDL_KeyboardEvent key)
{
  SDL_Keysym k;

  k = key.keysym;

  if(k.sym == SDLK_RETURN) {
    if(edit_finish(win_get_exwin()) == EDIT_SUCCESS) {
      win_set_exwin(EDIT_EXIT);
      keymode = KEY_NORMAL;
    }
  } else if(k.sym == SDLK_ESCAPE) {
    win_set_exwin(EDIT_EXIT);
    keymode = KEY_NORMAL;
  } else if(k.sym == SDLK_LEFT) {
    edit_move(EDIT_LEFT);
  } else if(k.sym == SDLK_RIGHT) {
    edit_move(EDIT_RIGHT);
  } else if(k.sym == SDLK_BACKSPACE) {
    edit_remove_char();
  }

  display_swap_screen();
  return 0;
}

static int debug_dispatch_keys(SDL_Event ev)
{
  switch(keymode) {
  case KEY_NORMAL:
    return debug_do_key_normal(ev.key);
  case KEY_EDIT:
    if(ev.type == SDL_TEXTINPUT) {
      return debug_do_text_edit(ev.text);
    } else if(ev.type == SDL_KEYDOWN) {
      return debug_do_key_edit(ev.key);
    }
  }
  return 0;
}

int debug_event_parse(SDL_Event ev)
{
  switch(ev.type) {
  case SDL_WINDOWEVENT:
    if(ev.window.windowID == debug_window_id) {
      screen_swap(DEBUG_NORMAL);
      display_swap_screen();
    }
    break;
  case SDL_TEXTINPUT:
  case SDL_KEYDOWN:
    if(debug_dispatch_keys(ev)) {
      SDL_Quit();
      return 1;
    }
    break;
  case SDL_QUIT:
    SDL_Quit();
    return 1;
  }

  return 0;
}

int debug_short_event()
{
  SDL_Event ev;
  
  if(!SDL_PollEvent(&ev)) {
    return 0;
  }

  switch(ev.type) {
  case SDL_KEYDOWN:
    printf("DEBUG: cpu->pc == %08x\n", cpu->pc);
    break;
  case SDL_QUIT:
    return DEBUG_QUIT;
  }

  return 0;
}

LONG debug_win_addr(int wnum)
{
  return win[wnum].addr;
}
