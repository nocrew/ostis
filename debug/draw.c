#include "common.h"
#include "display.h"

void draw_string(int x, int y, int f, char *str)
{
  if(!str) return;
  
  while(*str) {
    display_put_char(x, y, f, *str++);
    x += 8;
  }
}

void draw_filled(int x, int y, int w, int h, int c)
{
  int tx,ty;

  for(tx=0;tx<w;tx++) {
    for(ty=0;ty<h;ty++) {
      display_put_pixel_screen(tx+x, ty+y, c);
    }
  }
}

void draw_frame(int x, int y, int w, int h, int c)
{
  int tx,ty;

  for(tx=0;tx<w;tx++) {
    display_put_pixel_screen(tx+x, y, c);
    display_put_pixel_screen(tx+x, y+h-1, c);
  }
  for(ty=0;ty<h;ty++) {
    display_put_pixel_screen(x, ty+y, c);
    display_put_pixel_screen(x+w-1, ty+y, c);
  }
}

void draw_message(char *str)
{
  draw_string(8, 384, FONT_SMALL, str);
}

void draw_title(int x, int y, char *title, int selected)
{
  draw_string(8+x, y, FONT_SMALL+selected*2, title);
  if(title[strlen(title)-1] != ' ') {
    display_put_char(8+x+strlen(title)*8, y, FONT_SMALL+selected*2, ' ');
  }
}

void draw_cursor(int x, int y, int ch)
{
  display_put_char(x, y, FONT_LGINV, ch);
}


