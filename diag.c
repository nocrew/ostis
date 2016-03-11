#include <stdio.h>
#include <stdarg.h>
#include "common.h"
#include "cpu.h"
#include "mmu.h"

#define MAXDIAG 10000

static const char *level_name[] = {
  "",
  "FATAL",
  "ERROR",
  "WARNING",
  "INFO",
  "DEBUG",
  "TRACE"
};

struct diag_module {
  struct diag_module *next;
  char *id;
  int verbosity_limit;
};

static struct diag_module *modules = NULL;

static void add_module(char *module_str)
{
  struct diag_module *new;
  char *tmp;
  new = (struct diag_module *)malloc(sizeof(struct diag_module));

  if(!module_str || module_str[0] == '\0') return;
  
  if(module_str[0] == '-') {
    if(!strchr(module_str, ':')) {
      new->verbosity_limit = 1;
    } else {
      printf("ERROR! Cannot have both -ID and explicit level at once\n");
      exit(-1);
    }
    module_str++;
  }
  
  tmp = strchr(module_str, ':');
  if(tmp) {
    tmp[0] = '\0';
    tmp++;
    new->verbosity_limit = atoi(tmp);
    if(new->verbosity_limit < 1) {
      new->verbosity_limit = 1;
    }
  }
  new->id = module_str;
  new->next = modules;

  modules = new;
}

void diag_set_module_levels(char *defstr)
{
  char *str;
  char *tmp;

  if(!defstr || defstr[0] == '\0') {
    return;
  }
  str = strdup(defstr);
  
  while(1) {
    tmp = strchr(str, ',');
    if(!tmp) break;
    tmp[0] = '\0';
    tmp++;
    add_module(str);
    str = tmp;
  }
  add_module(str);
}

static int find_module_limit(char *id)
{
  struct diag_module *tmp;

  tmp = modules;
  
  while(tmp) {
    if(!strncmp(tmp->id, id, 4)) {
      return tmp->verbosity_limit;
    }
    tmp = tmp->next;
  }
  return MAXDIAG;
}

void print_diagnostic(int level, struct mmu *device, const char *format, ...)
{
  va_list args;
  int module_limit;
  char *id;
  int verbosity;

  if(device == NULL) {
    id = "";
    verbosity = 3;
  } else {
    id = device->id;
    verbosity = device->verbosity;
  }

  /* MAXDIAG means we have no special setting for that module, so obey default
   * limit in that case. If we DO find a special setting, use that, regardless
   * of the default value */
  module_limit = find_module_limit(id);
  if(module_limit == MAXDIAG) {
    if(level > verbosity)
      return;
  } else {
    if(level > module_limit)
      return;
  }
  
  fprintf(stderr, "%s", level_name[level]);
  if(level == 6) {
    fprintf(stderr, " [$%06x]", cpu->pc);
  }
  fprintf(stderr, ": ");
  if(device)
    fprintf(stderr, "%c%c%c%c: ", device->id[0], device->id[1],
	    device->id[2], device->id[3]);
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputc('\n', stderr);

  // Fatal error, abort immediately.
  if(level == 1)
    exit(1);
}

void diagnostics_level(struct mmu *device, int n)
{
  device->diagnostics(device, n);
}
