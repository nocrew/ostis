#ifndef SCREEN_H
#define SCREEN_H

#define BORDER_SIZE		50

void screen_init();
void screen_make_texture(const char *scale);
void screen_putpixel(int, int, long);
void screen_copyimage(unsigned char *);
void screen_swap(int);
void screen_clear();
void screen_disable(int);
int screen_check_disable();
void screen_toggle_grab();
void screen_toggle_fullscreen();
void screen_draw(int, int, int);
void screen_vsync(void);
void screen_hsync(void);
int screen_framecnt(int);
float screen_fps();
int screen_get_vsync(void);
void screen_set_delay(int);

extern int screen_window_id;

#define SDL_SCALING_NEAREST "0"
#define SDL_SCALING_LINEAR  "1"

#endif
