#include <stdio.h>
#include <unistd.h>
#include "mmu.h"
#include "ram.h"
#include "rom.h"
#include "cpu.h"
#include "psg.h"
#include "midi.h"
#include "ikbd.h"
#include "rtc.h"
#include "fdc.h"
#include "mfp.h"
#include "shifter.h"
#include "screen.h"
#include "floppy.h"
#include "debug/debug.h"
#include "cartridge.h"

#include "tests/tests.h"

/**
 * The type for a test case
 */
typedef int TestFunc(void);

typedef struct {
  char *name;
  TestFunc *func;
} TestCase;

static TestCase tests[] = {
  { "move immediate", test_move_immediate }
};

static void init_test( void )
{
  ram_clear();
}

int main( void )
{
  mmu_init();
  ram_init();
  rom_init();
  cpu_init();

  int c;
  for( c = 0 ; c < sizeof(tests) / sizeof(TestCase) ; c++ ) {
    init_test();

    printf( "running test: %s: ", tests[c].name );
    fflush( stdout );

    int ret = tests[c].func();
    if( ret == 0 ) {
      printf( "success\n" );
    }
    else {
      printf( "failed (error code %d)\n", ret );
    }
  }

  return 0;
}
