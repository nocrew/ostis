#ifndef DISPLAY_H
#define DISPLAY_H

#define FONT_SMALL 0
#define FONT_LARGE 1
#define FONT_SMINV 2
#define FONT_LGINV 3

void display_setup();
void display_clear_screen();
void display_put_char(int, int, int, int);
void display_put_pixel_screen(int, int, long);
void display_swap_screen();

#endif
