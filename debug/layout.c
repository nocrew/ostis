#include "common.h"
#include "cprint.h"
#include "win.h"
#include "layout.h"
#include "shifter.h"
#include "display.h"
#include "mmu.h"
#include "draw.h"

struct layout_window layout[LAYOUT_MAX] = {
  {   0,   0, 640, 394, 24, 78,  0,  1 }, /* Fullsize */
  {   0,   0, 640, 170, 10, 78,  0,  1 },
  {   0, 170, 440,  90,  5, 53,  0,  0 },
  { 440, 170, 200,  90,  5, 23,  0,  0 },
  {   0, 260, 440, 122,  7, 53,  0,  0 },
  { 440, 260, 200, 122,  7, 23,  0,  0 },
  {   0, 170, 440, 202, 12, 53,  0,  0 }, 
  {   0,   0,   0,   0,  0,  0,  0,  0 }, /* unused */
  { 440, 170, 200, 202, 12, 23,  0,  0 },
  {   0,   0,   0,   0,  0,  0,  0,  0 }, /* unused */
  {  40, 320, 552,  74,  0, 59, 40, 25 }
};

void layout_draw_window(int lnum, char *title, int selected)
{
  struct layout_window l;
  
  l = layout[lnum];

  draw_filled(l.x, l.y+8, l.w, l.h-8, WHITE);
  draw_frame(l.x+3, l.y+8, l.w-7, l.h-8, BLACK);
  draw_title(l.x, l.y, title, selected);
}

static int drawable(int c)
{
  if(!c) return 250;
  return c;
}

void layout_draw_info(int lnum, int wnum)
{
  int i,j,lcnt;
  static char text[80];
  int rows,font;
  int choff;
  struct cprint *cprint;

  lcnt = 0;
  rows = layout[lnum].rows;
  font = win[wnum].font;
  if(font == FONT_SMALL) rows *= 2;

  for(i=0;i<8;i++) {
    sprintf(text, "d%d = %08X  %c%c%c%c    "
	    "a%d = %08X %04X %04X %04X %04X %04X  %c%c%c%c%c%c%c%c%c%c",
	    i, cpu->d[i],
	    drawable((cpu->d[i]>>24)&0xff),
	    drawable((cpu->d[i]>>16)&0xff),
	    drawable((cpu->d[i]>>8)&0xff),
	    drawable(cpu->d[i]&0xff),
	    i, cpu->a[i],
	    bus_read_word_print(cpu->a[i]),
	    bus_read_word_print(cpu->a[i]+2),
	    bus_read_word_print(cpu->a[i]+4),
	    bus_read_word_print(cpu->a[i]+6),
	    bus_read_word_print(cpu->a[i]+8),
	    drawable(bus_read_byte_print(cpu->a[i])),
	    drawable(bus_read_byte_print(cpu->a[i]+1)),
	    drawable(bus_read_byte_print(cpu->a[i]+2)),
	    drawable(bus_read_byte_print(cpu->a[i]+3)),
	    drawable(bus_read_byte_print(cpu->a[i]+4)),
	    drawable(bus_read_byte_print(cpu->a[i]+5)),
	    drawable(bus_read_byte_print(cpu->a[i]+6)),
	    drawable(bus_read_byte_print(cpu->a[i]+7)),
	    drawable(bus_read_byte_print(cpu->a[i]+8)),
	    drawable(bus_read_byte_print(cpu->a[i]+9))
	    );
    draw_string(8, 9+8*(font+1)*i, font, text);
    lcnt++;
  }
  sprintf(text, "SR:%04X      %c%c%c%c%c%c%c", cpu->sr,
	  CHKT?'T':' ',
	  CHKX?'X':' ',
	  CHKS?'S':' ',
	  CHKN?'N':' ',
	  CHKZ?'Z':' ',
	  CHKV?'V':' ',
	  CHKC?'C':' ');
  draw_string(8, 9+8*(font+1)*lcnt, font, text);
  sprintf(text, "Raster beam: %d", screen_get_vsync());
  draw_string(400, 9+8*(font+1)*lcnt, font, text);
  lcnt++;
  cprint = cprint_instr(cpu->pc);
  if(!cprint) printf("DEBUG: ERROR!!!!\n");
  sprintf(text, "PC:%08X  %s %s", cpu->pc, cprint->instr, cprint->data);
  draw_string(8, 9+8*(font+1)*lcnt, font, text);
  free(cprint);
  sprintf(text, "Last cycle count: %d", cpu->icycle);
  draw_string(400, 9+8*(font+1)*lcnt, font, text);
  lcnt+=2;
  
  if(rows > lcnt) {
    sprintf(text, "ssp = %08X", cpu->ssp);
    draw_string(8, 9+8*(font+1)*lcnt, font, text);
  }
  lcnt += 2;
  for(i=0;i<MIN(10, rows-lcnt);i++) {
    sprintf(text, "m%d = %08X %04X %04X %04X %04X %04X %04X %04X %04X  ",
	    i, win[i].addr,
	    bus_read_word_print(win[i].addr),
	    bus_read_word_print(win[i].addr+2),
	    bus_read_word_print(win[i].addr+4),
	    bus_read_word_print(win[i].addr+6),
	    bus_read_word_print(win[i].addr+8),
	    bus_read_word_print(win[i].addr+10),
	    bus_read_word_print(win[i].addr+12),
	    bus_read_word_print(win[i].addr+14));
    choff = strlen(text);
    for(j=0;j<16;j++) {
      text[j+choff] = drawable(bus_read_byte_print(win[i].addr+j));
    }
    text[choff+16] = '\0';
    draw_string(8, 9+8*(font+1)*(lcnt+i), font, text);
  }
}

void layout_draw_dis(int lnum, int wnum)
{
  int i,off,brkcnt;
  static char text[1024];
  int rows, font;
  struct cprint *cprint;
  char *label;

  rows = layout[lnum].rows;
  font = win[wnum].font;

  if(font == FONT_SMALL) rows *= 2;
  off = 0;

  /*
   * This part makes sure auto-labeling has been executed before
   * printing, so that labels are shown properly
   */
  for(i=0;i<rows;i++) {
    cprint = cprint_instr(win[wnum].addr+off);
    off += cprint->size;
    free(cprint);
  }

  off = 0;

  for(i=0;i<rows;i++) {
    cprint = cprint_instr(win[wnum].addr+off);
    label = cprint_find_label(win[wnum].addr+off);
    brkcnt = cpu_find_breakpoint_lowest_cnt(win[wnum].addr+off);
    sprintf(text, "%06X  %-14s %c%s %s %c%d%c",
	    win[wnum].addr+off,
	    label?label:"",
	    (cpu->pc == win[wnum].addr+off)?3:' ',
	    cprint->instr,
	    cprint->data,
	    brkcnt?'[':0,
	    brkcnt?brkcnt:0,
	    brkcnt?']':0);
    draw_string(8+layout[lnum].x, 9+layout[lnum].y+8*(font+1)*i, font, text);
    off += cprint->size;
    free(cprint);
  }
}

static char *layout_mem_line(LONG addr, int col)
{
  static char text[80];
  int i,chcnt,choff;

  chcnt = 0;

  if(addr&1) {
    if(col == 78) {
      sprintf(text, "%06X %02X %04X %04X %04X %04X %04X %04X %04X %02X ",
	      addr&0xffffff,
	      bus_read_byte_print(addr),
	      bus_read_word_print(addr+1),
	      bus_read_word_print(addr+3),
	      bus_read_word_print(addr+5),
	      bus_read_word_print(addr+7),
	      bus_read_word_print(addr+9),
	      bus_read_word_print(addr+11),
	      bus_read_word_print(addr+13),
	      bus_read_byte_print(addr+15));
      chcnt = 16;
    } else if(col == 53) {
      sprintf(text, "%06X %02X %04X %04X %04X %04X %04X %02X ",
	      addr&0xffffff,
	      bus_read_byte_print(addr),
	      bus_read_word_print(addr+1),
	      bus_read_word_print(addr+3),
	      bus_read_word_print(addr+5),
	      bus_read_word_print(addr+7),
	      bus_read_word_print(addr+9),
	      bus_read_byte_print(addr+11));
      chcnt = 12;
    } else if(col == 23) {
      sprintf(text, "%06X %02X %04X %02X ",
	      addr&0xffffff,
	      bus_read_byte_print(addr),
	      bus_read_word_print(addr+1),
	      bus_read_byte_print(addr+3));
      chcnt = 4;
    }
  } else {
    if(col == 78) {
      sprintf(text, "%06X %04X %04X %04X %04X %04X %04X %04X %04X  ",
	      addr&0xffffff,
	      bus_read_word_print(addr),
	      bus_read_word_print(addr+2),
	      bus_read_word_print(addr+4),
	      bus_read_word_print(addr+6),
	      bus_read_word_print(addr+8),
	      bus_read_word_print(addr+10),
	      bus_read_word_print(addr+12),
	      bus_read_word_print(addr+14));
      chcnt = 16;
    } else if(col == 53) {
      sprintf(text, "%06X %04X %04X %04X %04X %04X %04X  ",
	      addr&0xffffff,
	      bus_read_word_print(addr),
	      bus_read_word_print(addr+2),
	      bus_read_word_print(addr+4),
	      bus_read_word_print(addr+6),
	      bus_read_word_print(addr+8),
	      bus_read_word_print(addr+10));
      chcnt = 12;
    } else if(col == 23) {
      sprintf(text, "%06X %04X %04X  ",
	      addr&0xffffff,
	      bus_read_word_print(addr),
	      bus_read_word_print(addr+2));
      chcnt = 4;
    }
  }

  choff = strlen(text);
  
  for(i=0;i<chcnt;i++) {
    text[i+choff] = drawable(bus_read_byte_print(addr+i));
  }
  text[choff+chcnt] = '\0';

  return text;
}

void layout_draw_mem(int lnum, int wnum)
{
  int i;
  int rows,font,linemult;

  rows = layout[lnum].rows;
  font = win[wnum].font;
  linemult = 0;

  switch(layout[lnum].cols) {
  case 78:
    linemult = 16;
    break;
  case 53:
    linemult = 12;
    break;
  case 23:
    linemult = 4;
    break;
  }

  if(font == FONT_SMALL) rows *= 2;

  for(i=0;i<rows;i++) {
    draw_string(layout[lnum].x+8, layout[lnum].y+9+8*(font+1)*i, font,
		layout_mem_line(win[wnum].addr+i*linemult,
				layout[lnum].cols));
  }
}
