// Include this in every MMU-connected device to enable diagnostics.
//
// Add HANDLE_DIAGNOSTICS near the top of the device source file to
// insert support code for diagnostics.  It will create a function
// which implements a callback to control the verbosity level.  This
// function must be included in the struct passed to mmu_register.
//
// The syntax for diagnostic output is this:
//   LEVEL(FORMAT [, ARGS])
// where LEVEL is one of the six levels below,
// FORMAT is a printf-style format string, and
// ARGS are arguments to the format string.

#include "mmu.h"

extern void diag_set_module_levels(char *);
extern void print_diagnostic(int, struct mmu *, const char *, ...);

// Panic, we're about to die!
#define FATAL(FORMAT, ...)  print_diagnostic(1, mmu_device, FORMAT, ##__VA_ARGS__)
// Serous error, but we can proceed.
#define ERROR(FORMAT, ...)  print_diagnostic(2, mmu_device, FORMAT, ##__VA_ARGS__)
// Problem, alert the user.
#define WARN(FORMAT, ...)   print_diagnostic(3, mmu_device, FORMAT, ##__VA_ARGS__)
// No problem, but user may want to know.
#define INFO(FORMAT, ...)   print_diagnostic(4, mmu_device, FORMAT, ##__VA_ARGS__)
// Step-by-step description of internal mechanisms.
#define DEBUG(FORMAT, ...)  print_diagnostic(5, mmu_device, FORMAT, ##__VA_ARGS__)
// Feel free to call this just about every cycle.
#define TRACE(FORMAT, ...)  print_diagnostic(6, mmu_device, FORMAT, ##__VA_ARGS__)

#define HANDLE_DIAGNOSTICS(device)				\
static struct mmu *mmu_device;					\
static void device ## _diagnostics(struct mmu *device, int n)	\
{								\
  mmu_device = device;						\
  device->verbosity = n;					\
}

#define HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(device, device_id)    \
  do {                                                          \
    extern void diagnostics_level(struct mmu *, int);           \
    extern int verbosity;                                       \
    struct mmu *dummy_device;                                   \
    dummy_device = (struct mmu *)malloc(sizeof(struct mmu));    \
    dummy_device->diagnostics = device ## _diagnostics;         \
    memcpy(dummy_device->id, device_id, 4);                     \
    diagnostics_level(dummy_device, verbosity);                 \
  } while(0)
