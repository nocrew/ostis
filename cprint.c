#include "cpu.h"
#include "cprint.h"
#include "mmu.h"

instr_print_t *instr_print[65536];

static struct cprint_label *clabel = NULL;

struct cprint *cprint_alloc(LONG addr)
{
  struct cprint *ret;

  ret = xmalloc(sizeof(struct cprint));
  ret->addr = addr;
  ret->instr[0] = '\0';
  ret->data[0] = '\0';
  ret->hex[0] = '\0';
  ret->extra[0] = '\0';
  ret->size = 2;
  
  return ret;
}

void cprint_free(struct cprint *in)
{
  free(in);
}


static char *cprint_find_auto_label(LONG addr)
{
  struct cprint_label *t;

  t = clabel;

  while(t) {
    if(!t->named && (t->addr == (addr&0xffffff))) return t->name;
    t = t->next;
  }

  return NULL;
}

void cprint_set_label(LONG addr, char *name)
{
  struct cprint_label *new;
  static char tname[10];
  char *tmp;

  if(!debugger)
    return;

  if(name) {
    tmp = cprint_find_label(addr);
    if(tmp && !strcmp(tmp, name)) return;
  } else {
    tmp = cprint_find_auto_label(addr);
    sprintf(tname, "L_%x", addr&0xffffff);
    if(tmp && !strcmp(tmp, tname)) return;
  }

  new = xmalloc(sizeof(struct cprint_label));
  new->addr = addr&0xffffff;
  if(name) {
    new->name = name;
    new->named = 1;
  } else {
    new->name = xmalloc(10);
    sprintf(new->name, "L_%x", addr&0xffffff);
    new->named = 0;
  }
  new->next = clabel;
  clabel = new;
}

int cprint_label_exists(char *name)
{
  struct cprint_label *t;
  
  t = clabel;

  while(t) {
    if(!strcasecmp(t->name, name)) return 1;
    t = t->next;
  }
  
  return 0;
}

LONG cprint_label_addr(char *name)
{
  struct cprint_label *t;
  
  t = clabel;
  
  while(t) {
    if(!strcasecmp(t->name, name)) return t->addr;
    t = t->next;
  }
  
  return 0;
}

char *cprint_find_label(LONG addr)
{
  struct cprint_label *t;

  t = clabel;

  while(t) {
    if(t->named && (t->addr == (addr&0xffffff))) return t->name;
    t = t->next;
  }

  t = clabel;

  while(t) {
    if(t->addr == (addr&0xffffff)) return t->name;
    t = t->next;
  }

  return NULL;
}

void cprint_save_labels(char *file)
{
  FILE *f;
  struct cprint_label *t;

  f = fopen(file, "wct");
  if(!f) {
    perror("fopen");
  }
  
  t = clabel;

  while(t) {
    if(t->named) {
      fprintf(f, "%-14s %08X\n", t->name, t->addr);
    }
    t = t->next;
  }
  fclose(f);
}

void cprint_load_labels(char *file)
{
  FILE *f;
  char name[1024];
  LONG addr;

  f = fopen(file, "rb");
  if(!f) {
    perror("fopen");
  }

  while(!feof(f)) {
    if(fscanf(f, "%14s %08X\n", name, &addr) != 2)
      WARNING(fscanf);
    cprint_set_label(addr, xstrdup(name));
  }
  
  fclose(f);
}

struct cprint *cprint_instr(LONG addr)
{
  WORD op;

  if(cpu->pc == addr) {
    if(cpu->has_prefetched) {
      op = cpu->prefetched_instr;
    } else {
      op = mmu_read_word_print(addr);
    }
  } else {
    op = mmu_read_word_print(addr);
  }
  
  return instr_print[op](addr, op);
}

