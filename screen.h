#ifndef SCREEN_H
#define SCREEN_H

#define BORDER_SIZE		50

void screen_init();
void screen_putpixel(int, int, long);
void screen_copyimage(unsigned char *);
void screen_swap();
void screen_clear();
void screen_disable(int);
int screen_check_disable();
void *screen_pixels();

#endif
