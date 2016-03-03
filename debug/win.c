#include "common.h"
#include "cprint.h"
#include "win.h"
#include "display.h"
#include "edit.h"
#include "draw.h"
#include "layout.h"

struct win win[10];

static int selwin = 0;
static int exwin = 0;
static char mesg[80];

static char *win_get_title(int wnum)
{
  static char tmp[100];

  tmp[0] = win[wnum].num + '0';
  tmp[1] = ' ';
  tmp[2] = '\0';
  strcat(tmp, win[wnum].title);
  return tmp;
}

void win_draw_window(int lnum, int wnum)
{
  layout_draw_window(lnum, win_get_title(wnum), (selwin==wnum)?1:0);

  switch(win[wnum].type) {
  case TYPE_INFO:
    layout_draw_info(lnum, wnum);
    break;
  case TYPE_DIS:
    layout_draw_dis(lnum, wnum);
    break;
  case TYPE_MEM:
    layout_draw_mem(lnum, wnum);
    break;
  }
}

static int win_visible(int wnum)
{
  int i;

  if(win[wnum].pos == LAYOUT_FULL) return 1;
  for(i=0;i<10;i++) {
    if(win[i].pos == LAYOUT_FULL) return 0;
  }
  if((win[0].pos == LAYOUT_INFO) && (wnum == 0)) return 1;
  if((win[2].pos == LAYOUT_WIN24) && (wnum == 2)) return 1;
  if((win[4].pos == LAYOUT_WIN24) && (wnum == 4)) return 1;
  if((win[3].pos == LAYOUT_WIN35) && (wnum == 3)) return 1;
  if((win[5].pos == LAYOUT_WIN35) && (wnum == 5)) return 1;
  if((win[2].pos == LAYOUT_WIN2) && (win[4].pos == LAYOUT_WIN4))
    if((wnum == 2) || (wnum == 4)) return 1;
  if((win[3].pos == LAYOUT_WIN3) && (win[5].pos == LAYOUT_WIN5))
    if((wnum == 3) || (wnum == 5)) return 1;
  return 0;
}

void win_set_message(char *text)
{
  strcpy(mesg, text);
}

void win_cycle_selected()
{
  do {
    selwin++;
    if(selwin > 9) selwin = 0;
  } while(!win_visible(selwin));
}

void win_set_selected(int wnum)
{
  selwin = wnum;
}

int win_get_selected()
{
  return selwin;
}

void win_set_exwin(int ex)
{
  exwin = ex;
}

int win_get_exwin()
{
  return exwin;
}

void win_setup(int wnum, int num, int pos, int type,
	       int font, char *title, LONG addr)
{
  win[wnum].num = num;
  win[wnum].pos = pos;
  win[wnum].type = type;
  win[wnum].font = font;
  win[wnum].addr = addr;
  strcpy(win[wnum].title, title);
}

void win_setup_default()
{
  selwin = 0;
  static char title[80];

  sprintf(title, "Registers        oSTis 0.80 %c NoCrew 2016", 189);
  win_setup(0, 1, 1, 0, 1, title, 0);
  win_setup(1, 1, 7, 2, 1, "Memory", 0);
  win_setup(2, 2, 6, 1, 1, "Disassembly pc", 0xfc0020);
  win_setup(3, 3, 8, 2, 1, "Memory", 0);
  win_setup(4, 4, 4, 1, 1, "Disassembly", 0);
  win_setup(5, 5, 5, 2, 1, "Memory", 0);
  win_setup(6, 6, 7, 2, 1, "Memory", 0);
  win_setup(7, 7, 7, 2, 1, "Memory", 0);
  win_setup(8, 8, 7, 2, 1, "Memory", 0);
  win_setup(9, 9, 7, 2, 1, "Memory", 0);
}

void win_draw_screen()
{
  int i;
  int full,used24, used35;
  
  full = used24 = used35 = -1;

  for(i=0;i<10;i++) {
    if(win[i].pos == LAYOUT_FULL) full = i;
    if(win[i].pos == LAYOUT_WIN24) used24 = i;
    if(win[i].pos == LAYOUT_WIN35) used35 = i;
  }

  display_clear_screen();
  if(full != -1) {
    win_draw_window(LAYOUT_FULL, full);
  } else {
    win_draw_window(LAYOUT_INFO, 0);
    if(used24 != -1) {
      win_draw_window(LAYOUT_WIN24, used24);
    } else {
      win_draw_window(LAYOUT_WIN2, 2);
      win_draw_window(LAYOUT_WIN4, 4);
    }
    if(used35 != -1) {
      win_draw_window(LAYOUT_WIN35, used35);
    } else {
      win_draw_window(LAYOUT_WIN3, 3);
      win_draw_window(LAYOUT_WIN5, 5);
    }
  }
  if(exwin) {
    edit_draw_window(exwin);
  } else {
    draw_message(mesg);
  }
}

static int rows_per_page(int wnum)
{
  int rows;

  rows = layout[win[selwin].pos].rows;
  if(win[selwin].font == FONT_SMALL) rows *= 2;

  return rows;
}

static int bytes_per_row_mem(int wnum)
{
  switch(layout[win[selwin].pos].cols) {
  case 78:
    return 16;
  case 53:
    return 12;
  case 23:
    return 4;
  }
  return 0;
}

static int pc_on_screen()
{
  int i,off;
  struct cprint *cprint;

  off = 0;

  for(i=0;i<rows_per_page(selwin);i++) {
    if((win[selwin].addr+off) == cpu->pc) {
      return 1;
    }
    cprint = cprint_instr(win[selwin].addr+off);
    off += cprint->size;
    free(cprint);
  }
  
  return 0;
}

static void win_move_dis(int dir, int page)
{
  int i;
  struct cprint *cprint;
  
  if(page) {
    switch(dir) {
    case MOVE_UP:
      win[selwin].addr -= 2*rows_per_page(selwin);
      break;
    case MOVE_DOWN:
      for(i=0;i<rows_per_page(selwin);i++) {
	cprint = cprint_instr(win[selwin].addr);
	win[selwin].addr += cprint->size;
	free(cprint);
      }
      break;
    case MOVE_LEFT:
      break;
    case MOVE_RIGHT:
      break;
    }
  } else {
    switch(dir) {
    case MOVE_UP:
      win[selwin].addr -= 2;
      break;
    case MOVE_DOWN:
      cprint = cprint_instr(win[selwin].addr);
      win[selwin].addr += cprint->size;
      free(cprint);
      break;
    case MOVE_LEFT:
      win[selwin].addr -= 2;
      break;
    case MOVE_RIGHT:
      win[selwin].addr += 2;
      break;
    }
  }
}

static void win_move_mem(int dir, int page)
{
  if(page) {
    switch(dir) {
    case 0:
      win[selwin].addr -= bytes_per_row_mem(selwin)*rows_per_page(selwin);
      break;
    case 1:
      win[selwin].addr += bytes_per_row_mem(selwin)*rows_per_page(selwin);
      break;
    case 2:
      break;
    case 3:
      break;
    }
  } else {
    switch(dir) {
    case 0:
      win[selwin].addr -= bytes_per_row_mem(selwin);
      break;
    case 1:
      win[selwin].addr += bytes_per_row_mem(selwin);
      break;
    case 2:
      win[selwin].addr -= 1;
      break;
    case 3:
      win[selwin].addr += 1;
      break;
    }
  }
}

void win_move_window(int dir, int page)
{
  if(win[selwin].type == TYPE_INFO) {
    win[2].addr = cpu->pc;
  } else if(win[selwin].type == TYPE_DIS) {
    win_move_dis(dir, page);
  } else if(win[selwin].type == TYPE_MEM) {
    win_move_mem(dir, page);
  }
}

void win_move_window_to_pc()
{
  if(!pc_on_screen()) {
    win[selwin].addr = cpu->pc;
  }
}

void win_toggle_window_type(int wnum)
{
  if(wnum == -1) wnum = selwin;

  if((wnum == 2) || (wnum == 4)) {
    if(win[wnum].type == TYPE_DIS) {
      win[wnum].type = TYPE_MEM;
      if(wnum == 2)
        strcpy(win[2].title, "Memory pc");
      else
        strcpy(win[4].title, "Memory");
    } else if(win[wnum].type == TYPE_MEM) {
      win[wnum].type = TYPE_DIS;
      win[wnum].addr = (win[wnum].addr+1)&0xfffffe;
      if(wnum == 2)
        strcpy(win[2].title, "Disassembly pc");
      else
        strcpy(win[4].title, "Disassembly");
    }
  }
}

void win_toggle_window_font(int wnum)
{
  if(wnum == -1) wnum = selwin;

  if(win[wnum].font == FONT_SMALL)
    win[wnum].font = FONT_LARGE;
  else if(win[wnum].font == FONT_LARGE)
    win[wnum].font = FONT_SMALL;
}

void win_toggle_window_split(int wnum)
{
  if(wnum == -1) wnum = selwin;

  if(win[wnum].pos == LAYOUT_WIN24) win[wnum].pos = wnum;
  else if(win[wnum].pos == LAYOUT_WIN2) win[wnum].pos = LAYOUT_WIN24;
  else if(win[wnum].pos == LAYOUT_WIN4) win[wnum].pos = LAYOUT_WIN24;
  else if(win[wnum].pos == LAYOUT_WIN35) win[wnum].pos = wnum;
  else if(win[wnum].pos == LAYOUT_WIN3) win[wnum].pos = LAYOUT_WIN35;
  else if(win[wnum].pos == LAYOUT_WIN5) win[wnum].pos = LAYOUT_WIN35;
}

void win_toggle_window_full(int wnum)
{
  if(wnum == -1) wnum = selwin;

  if(win[wnum].pos) {
    win[wnum].lastpos = win[wnum].pos;
    win[wnum].pos = LAYOUT_FULL;
  } else {
    win[wnum].pos = win[wnum].lastpos;
  }
}
