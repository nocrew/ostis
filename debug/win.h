#ifndef WIN_H
#define WIN_H

#include "common.h"

struct win {
  LONG addr;
  int num;
  int pos;
  int lastpos;
  int type;
  int font;
  char title[80];
};

#define WIN_CURRENT -1

#define TYPE_INFO 0
#define TYPE_DIS  1
#define TYPE_MEM  2

#define MOVE_UP    0
#define MOVE_DOWN  1
#define MOVE_LEFT  2
#define MOVE_RIGHT 3

extern struct win win[10];

void win_draw_window(int, int);
void win_set_message(char *);
void win_cycle_selected();
void win_set_selected(int);
int win_get_selected();
void win_set_exwin(int);
int win_get_exwin();
void win_setup(int, int, int, int, int, char *, LONG);
void win_setup_default();
void win_draw_screen();
void win_move_window(int, int);
void win_move_window_to_pc();
void win_toggle_window_type(int);
void win_toggle_window_font(int);
void win_toggle_window_split(int);
void win_toggle_window_full(int);

#endif
