#ifndef CPRINT_H
#define CPRINT_H

struct cprint {
  char instr[16];
  char data[64];
  char hex[32];
  char extra[64];
  int size;
  LONG addr;
};

struct cprint_label {
  struct cprint_label *next;
  char *name;
  LONG addr;
  int named;
};

struct cprint *cprint_alloc(LONG);

typedef struct cprint *instr_print_t(LONG, WORD);
extern instr_print_t *instr_print[];

#endif
