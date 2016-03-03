#ifndef EDIT_H
#define EDIT_H

#define EDIT_EXIT     0
#define EDIT_SETADDR  1
#define EDIT_SETBRK   2
#define EDIT_LABEL    3
#define EDIT_LABELCMD 4
#define EDIT_SETWATCH 5
#define EDIT_SETREG   6
#define EDIT_SETMEM   7

#define EDIT_LEFT  0
#define EDIT_RIGHT 1

#define EDIT_SUCCESS 0
#define EDIT_FAILURE 1

void edit_draw_window(int);
void edit_setup();
void edit_insert_char(int);
int edit_finish();
void edit_move(int);
void edit_remove_char();

#endif
