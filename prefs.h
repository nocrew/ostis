#ifndef PREFS_H
#define PREFS_H

struct prefs {
  char *tosimage;
  char *stateimage;
  char *diskimage;
  char *cartimage;
};

extern struct prefs prefs;

void prefs_default();
void prefs_set(char *, char *);
void prefs_init();

#endif

