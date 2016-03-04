#include <stdio.h>
#include <stdarg.h>
#include "mmu.h"

static const char *level_name[] = {
  "",
  "FATAL",
  "ERROR",
  "WARNING",
  "INFO",
  "DEBUG",
  "TRACE"
};

void print_diagnostic(int level, struct mmu *device, const char *format, ...)
{
  va_list args;

  if(level > device->verbosity)
    return;

  fprintf(stderr, "%s: ", level_name[level]);
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
