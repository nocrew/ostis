#ifndef LAYOUT_H
#define LAYOUT_H

struct layout_window {
  int x,y;
  int w,h;
  int rows;
  int cols;
  int text_xoff;
  int text_yoff;
};

#define BLACK 0x000000
#define WHITE 0xffffff

#define LAYOUT_FULL      0
#define LAYOUT_INFO      1
#define LAYOUT_WIN2      2
#define LAYOUT_WIN3      3
#define LAYOUT_WIN4      4
#define LAYOUT_WIN5      5
#define LAYOUT_WIN24     6
#define LAYOUT_WIN35     8
#define LAYOUT_EDIT     10
#define LAYOUT_MAX      11

extern struct layout_window layout[LAYOUT_MAX];

void layout_draw_window(int, char *, int);
void layout_draw_info(int, int);
void layout_draw_dis(int, int);
void layout_draw_mem(int, int);

/*
 * Layout:
 * 11111111
 * 11111111
 * 22222333
 * 44444555
 *
 * vispos:
 * 0 == full (all)
 * 1 == 1 (only 0)
 * 2 == 2 (only 2)
 * 3 == 3 (only 3)
 * 4 == 4 (only 4)
 * 5 == 5 (only 5)
 * 6 == 2+4 (only 2 and 4)
 * 8 == 3+5 (only 3 and 5)
 * 
 * type:
 * 0 == registers (only 0)
 * 1 == disassembler (only 2 and 4)
 * 2 == memory (2-5)
 */

#endif
