#include "common.h"
#include "edit.h"
#include "display.h"
#include "draw.h"
#include "win.h"
#include "expr.h"
#include "layout.h"

struct edit {
  int pos;
  int len;
  char text[80];
};

static struct edit edit;

static void edit_draw_cursor()
{
  if(edit.pos > strlen(edit.text)) {
    edit.pos = strlen(edit.text);
  }
  draw_cursor(layout[LAYOUT_EDIT].x + 
	      layout[LAYOUT_EDIT].text_xoff +edit.pos*8, 
	      layout[LAYOUT_EDIT].y + layout[LAYOUT_EDIT].text_yoff + 16,
	      edit.text[edit.pos]);
}

void edit_draw_window(int exwin)
{
  char text[80];

  switch(exwin) {
  case EDIT_SETADDR:
    strcpy(text, "Window start address?");
    break;
  case EDIT_SETBRK:
    strcpy(text, "Breakpoint addr[,#-]");
    break;
  case EDIT_LABEL:
    strcpy(text, "Label name,addr");
    break;
  case EDIT_LABELCMD:
    strcpy(text, "Label [save|load],filename");
    break;
  case EDIT_SETWATCH:
    strcpy(text, "Watchpoint expr[=,!=,>,>=,<,<=]expr[,#]");
    break;
  case EDIT_SETREG:
    strcpy(text, "Set reg=expr");
    break;
  default:
    return;
  }

  layout_draw_window(LAYOUT_EDIT, "    ESC to abort     ", 0);
  draw_string(layout[LAYOUT_EDIT].x + layout[LAYOUT_EDIT].text_xoff,
	      layout[LAYOUT_EDIT].y + layout[LAYOUT_EDIT].text_yoff,
	      FONT_LARGE, text);
  draw_string(layout[LAYOUT_EDIT].x + layout[LAYOUT_EDIT].text_xoff,
	      layout[LAYOUT_EDIT].y + layout[LAYOUT_EDIT].text_yoff + 16,
	      FONT_LARGE, edit.text);
  edit_draw_cursor();
}

static int edit_do_setaddr()
{
  LONG addr;
  int selwin;

  selwin = win_get_selected();

  addr = 0;

  if(strlen(edit.text) == 0) return EDIT_FAILURE;

  if(expr_parse(&addr, edit.text) == EXPR_SUCCESS) {
    win[selwin].addr = addr;
    if(win[selwin].type == TYPE_DIS)
      win[selwin].addr = (win[selwin].addr+1)&0xfffffe;
    return EDIT_SUCCESS;
  }

  return EDIT_FAILURE;
}

static int edit_do_setbrk()
{
  LONG addr;
  int cnt;
  char text[80],*tmp;
  
  if(strlen(edit.text) == 0) return EDIT_FAILURE;
  if(strchr(edit.text, ',') != strrchr(edit.text, ',')) return EDIT_FAILURE;

  strcpy(text, edit.text);

  tmp = strchr(text, ',');
  if(tmp) {
    tmp[0] = '\0';
    tmp++;
    if(strlen(tmp) == 0) return EDIT_FAILURE;
    if(tmp[0] == '-') {
      cnt = 0;
    } else if((tmp[0] >= '0') && (tmp[0] <= '9')) {
      cnt = strtol(tmp, NULL, 10);
    } else {
      return EDIT_FAILURE;
    }
  } else {
    cnt = -1;
  }

  if(expr_parse(&addr, text) == EXPR_FAILURE)
    return EDIT_FAILURE;

  if(cnt == 0) {
    if(cpu_unset_breakpoint(addr))
      return EDIT_FAILURE;
    return EDIT_SUCCESS;
  } else if(cnt) {
    cpu_set_breakpoint(addr, cnt);
    return EDIT_SUCCESS;
  }
  
  return EDIT_FAILURE;
}

static int edit_do_setwatch()
{
  LONG addr;
  int cnt,mode;
  char text[80],*tmp;
  
  if(strlen(edit.text) == 0) return EDIT_FAILURE;
  if(strchr(edit.text, ',') != strrchr(edit.text, ',')) return EDIT_FAILURE;
  if(strchr(edit.text, '=') != strrchr(edit.text, '=')) return EDIT_FAILURE;
  if(strchr(edit.text, '>') != strrchr(edit.text, '>')) return EDIT_FAILURE;
  if(strchr(edit.text, '<') != strrchr(edit.text, '<')) return EDIT_FAILURE;

  strcpy(text, edit.text);

  tmp = strchr(text, ',');
  if(tmp) {
    tmp[0] = '\0';
    tmp++;
    if(strlen(tmp) == 0) return EDIT_FAILURE;
    if(tmp[0] == '-') {
      cnt = 0;
    } else if((tmp[0] >= '0') && (tmp[0] <= '9')) {
      cnt = strtol(tmp, NULL, 10);
    } else {
      return EDIT_FAILURE;
    }
  } else {
    cnt = 1;
  }

  tmp = strchr(text, '=');
  if(tmp) {
    tmp[0] = '\0';
    tmp++;
    if(strlen(tmp) == 0) return EDIT_FAILURE;

    mode = CPU_WATCH_EQ;

    if(text[strlen(text)-1] == '!') {
      mode = CPU_WATCH_NE;
      text[strlen(text)-1] = '\0';
    } else if(text[strlen(text)-1] == '<') {
      mode = CPU_WATCH_LE;
      text[strlen(text)-1] = '\0';
    } else if(text[strlen(text)-1] == '>') {
      mode = CPU_WATCH_GE;
      text[strlen(text)-1] = '\0';
    }
  } else {
    tmp = strchr(text, '>');
    if(tmp) {
      tmp[0] = '\0';
      tmp++;
      if(strlen(tmp) == 0) return EDIT_FAILURE;
      mode = CPU_WATCH_GT;
    } else {
      tmp = strchr(text, '<');
      if(tmp) {
	tmp[0] = '\0';
	tmp++;
	if(strlen(tmp) == 0) return EDIT_FAILURE;
	mode = CPU_WATCH_LT;
      } else {
	return EDIT_FAILURE;
      }
    }
  }

  if(expr_parse(&addr, text) == EXPR_FAILURE)
    return EDIT_FAILURE;
  if(expr_parse(&addr, tmp) == EXPR_FAILURE)
    return EDIT_FAILURE;

  switch(mode) {
  case CPU_WATCH_EQ:
    printf("DEBUG: \"%s\" == \"%s\"\n", text, tmp);
    break;
  case CPU_WATCH_NE:
    printf("DEBUG: \"%s\" != \"%s\"\n", text, tmp);
    break;
  case CPU_WATCH_GT:
    printf("DEBUG: \"%s\" > \"%s\"\n", text, tmp);
    break;
  case CPU_WATCH_GE:
    printf("DEBUG: \"%s\" >= \"%s\"\n", text, tmp);
    break;
  case CPU_WATCH_LT:
    printf("DEBUG: \"%s\" < \"%s\"\n", text, tmp);
    break;
  case CPU_WATCH_LE:
    printf("DEBUG: \"%s\" <= \"%s\"\n", text, tmp);
    break;
  }

  if(cnt == 0) {
    return EDIT_FAILURE;
  } else if(cnt) {
    cpu_set_watchpoint(strdup(text), strdup(tmp), cnt, mode);
    return EDIT_SUCCESS;
  }
  
  return EDIT_FAILURE;
}

static int edit_match_areg(char *text)
{
  if(((text[0] == 'a') || (text[0] == 'A')) &&
     ((text[1] >= '0') && (text[1] <= '7')))
    return 1;
  return 0;
}

static int edit_match_dreg(char *text)
{
  if(((text[0] == 'd') || (text[0] == 'D')) &&
     ((text[1] >= '0') && (text[1] <= '7')))
    return 1;
  return 0;
}

static int edit_match_win(char *text)
{
  if(((text[0] == 'm') || (text[0] == 'M')) &&
     ((text[1] >= '0') && (text[1] <= '9')))
    return 1;
  return 0;
}

static int edit_do_setreg()
{
  LONG addr;
  char text[80],*tmp;
  
  if(strlen(edit.text) == 0) return EDIT_FAILURE;
  if(strchr(edit.text, '=') != strrchr(edit.text, '=')) return EDIT_FAILURE;

  strcpy(text, edit.text);

  tmp = strchr(text, '=');
  if(!tmp) return EDIT_FAILURE;
  tmp[0] = '\0';
  tmp++;
  if(strlen(tmp) == 0) return EDIT_FAILURE;
  if((strlen(text) != 2) && 
     strcasecmp("ssp", text) &&
     strcasecmp("usp", text))
    return EDIT_FAILURE;

  if(expr_parse(&addr, tmp) == EXPR_FAILURE)
    return EDIT_FAILURE;

  if(!strcasecmp("ssp", text)) {
    cpu->ssp = addr;
    if(CHKS) cpu->a[7] = addr;
  } else if(!strcasecmp("usp", text)) {
    cpu->usp = addr;
    if(!CHKS) cpu->a[7] = addr;
  } else if(edit_match_areg(text)) {
    cpu->a[text[1]-'0'] = addr;
  } else if(edit_match_dreg(text)) {
    cpu->d[text[1]-'0'] = addr;
  } else if(edit_match_win(text)) {
    win[text[1]-'0'].addr = addr;
  } else if(!strcasecmp("pc", text)) {
    cpu->pc = addr;
  } else if(!strcasecmp("sr", text)) {
    cpu->sr = addr&0xffff;
  } else {
    return EDIT_FAILURE;
  }

  return EDIT_SUCCESS;
}

static int edit_do_label()
{
  LONG addr;
  char text[80],*tmp;
  
  if(strlen(edit.text) == 0) return EDIT_FAILURE;
  if(strchr(edit.text, ',') != strrchr(edit.text, ',')) return EDIT_FAILURE;

  strcpy(text, edit.text);

  tmp = strchr(text, ',');
  if(!tmp) return EDIT_FAILURE;

  tmp[0] = '\0';
  tmp++;
  if(strlen(tmp) == 0) return EDIT_FAILURE;

  if(expr_parse(&addr, tmp) == EXPR_FAILURE)
    return EDIT_FAILURE;

  cprint_set_label(addr, strdup(text));
  return EDIT_SUCCESS;
}

static int edit_do_labelcmd()
{
  char text[80],*tmp;
  
  if(strlen(edit.text) == 0) return EDIT_FAILURE;
  if(strchr(edit.text, ',') != strrchr(edit.text, ',')) return EDIT_FAILURE;

  strcpy(text, edit.text);

  tmp = strchr(text, ',');
  if(!tmp) return EDIT_FAILURE;

  tmp[0] = '\0';
  tmp++;
  if(strlen(tmp) == 0) return EDIT_FAILURE;
  if(strlen(text) == 0) return EDIT_FAILURE;

  if(!strncasecmp(text, "save", 4)) {
    cprint_save_labels(tmp);
    return EDIT_SUCCESS;
  } else if(!strncasecmp(text, "load", 4)) {
    cprint_load_labels(tmp);
    return EDIT_SUCCESS;
  }

  return EDIT_FAILURE;
}

void edit_move(int dir)
{
  if(dir == EDIT_RIGHT) {
    edit.pos++;
    if(edit.pos > strlen(edit.text)) edit.pos = strlen(edit.text);
  } else { /* EDIT_LEFT */
    edit.pos--;
    if(edit.pos < 0) edit.pos = 0;
  }
}

void edit_insert_char(int c)
{
  char tmp[80];
  if(strlen(edit.text) >= edit.len) return;
  
  strcpy(tmp, &edit.text[edit.pos]);
  edit.text[edit.pos] = c;
  strcpy(&edit.text[edit.pos+1], tmp);
  edit_move(EDIT_RIGHT);
}

void edit_remove_char()
{
  char tmp[80];
  if(edit.pos == 0) return;

  strcpy(tmp, &edit.text[edit.pos]);
  strcpy(&edit.text[edit.pos-1], tmp);
  edit_move(EDIT_LEFT);
}

int edit_finish(int exwin)
{
  switch(exwin) {
  case EDIT_SETADDR:
    return edit_do_setaddr();
  case EDIT_SETBRK:
    return edit_do_setbrk();
  case EDIT_LABEL:
    return edit_do_label();
  case EDIT_LABELCMD:
    return edit_do_labelcmd();
  case EDIT_SETWATCH:
    return edit_do_setwatch();
  case EDIT_SETREG:
    return edit_do_setreg();
  default:
    return EDIT_SUCCESS;
  }
}

void edit_setup()
{
  edit.pos = 0;
  edit.len = layout[LAYOUT_EDIT].cols;
  edit.text[0] = '\0';
}
