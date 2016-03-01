#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include "common.h"
#include "mmu.h"
#include "mmu_fallback.h"
#include "ram.h"
#include "rom.h"
#include "cpu.h"
#include "psg.h"
#include "midi.h"
#include "ikbd.h"
#if INCLUDE_RTC
#include "rtc.h"
#endif
#include "fdc.h"
#include "mfp.h"
#include "prefs.h"
#include "shifter.h"
#include "screen.h"
#include "floppy.h"
#include "debug/debug.h"
#include "cartridge.h"
#include "state.h"
#if TEST_BUILD
#include "tests/test_main.h"
#endif

int debugger = 0;
int ppmoutput = 0;
int psgoutput = 0;
int vsync_delay = 0;
int play_audio = 0;
int monitor_sm124 = 0;

#if TEST_BUILD
int test_mode = 0;
char *test_case_name;
#define OPT_TEST_MODE            99999
#endif


#define OPT_CART                 10000
#define OPT_FORCE_EXTREME_DISASM 10001

int main(int argc, char *argv[])
{
  int c;
  struct state *state = NULL;
#if TEST_BUILD
  struct test_case *test_case;
#endif
  prefs_init();

  while(1) {
    int option_index = 0;
    static struct option long_options[] = {
      {"cart",                  required_argument, 0, OPT_CART },
      {"force-extreme-disasm",  no_argument,       0, OPT_FORCE_EXTREME_DISASM },
#if TEST_BUILD
      {"test-case",             required_argument, 0, OPT_TEST_MODE},
#endif
      {0,                       0,                 0, 0 }
    };
    c = getopt_long(argc, argv, "a:t:s:hdpyVAM", long_options, &option_index);
    if(c == -1) break;

    switch(c) {
    case 'a':
      prefs_set("diskimage", optarg);
      break;
    case 't':
      prefs_set("tosimage", optarg);
      break;
    case 's':
      prefs_set("stateimage", optarg);
      break;
    case OPT_CART:
      prefs_set("cartimage", optarg);
      break;
    case OPT_FORCE_EXTREME_DISASM:
      cprint_all = 1;
      break;
#if TEST_BUILD
    case OPT_TEST_MODE:
      test_case_name = strdup(optarg);
      test_mode = 1;
      break;
#endif
    case 'd':
      debugger = 1;
      break;
    case 'p':
      ppmoutput = 1;
      break;
    case 'y':
      psgoutput = 1;
      break;
    case 'V':
      vsync_delay = 1;
      break;
    case 'A':
      play_audio = 1;
      break;
    case 'M':
      monitor_sm124 = 1;
      break;
    case 'h':
    default:
      printf("Usage: %s [-a diskimage] [-t tosimage] [-s stateimage] [-h] [-d] [-p]\n",
	     argv[0]);
      exit(-1);
      break;
    }
  }

  if((prefs.diskimage == NULL) && (argv[optind] != NULL))
    prefs_set("diskimage", argv[optind]);

  SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK|SDL_INIT_AUDIO);

#if TEST_BUILD
  if(test_mode) {
    test_case = test_init(test_case_name);
    if(test_case) {
      if(test_case->cartridge_name) {
        prefs_set("cartimage", test_case->cartridge_name);
      }
      if(test_case->floppy_name) {
        prefs_set("diskimage", test_case->floppy_name);
      }
    } else {
      printf("DEBUG: Could not load test case %s\n", test_case_name);
      exit(-3);
    }
  }
#endif
  
  /* Must run before hardware module inits */
  mmu_init();

  /* This must also be run before hardware modules.
     It gives a dummy area for some memory regions to not
     cause bus errors */
  mmu_fallback_init(); 

  /* Actual hardware */
  ram_init();
  rom_init();
  cpu_init();
  if(prefs.cartimage) {
    cartridge_init(prefs.cartimage);
  } else {
    cartridge_init(NULL);
  }
  psg_init();
  midi_init();
  ikbd_init();
#if INCLUDE_RTC
  rtc_init();
#endif
  fdc_init();
  mfp_init();
  screen_disable(0);
  screen_init();
  shifter_init();
  if(debugger) {
    debug_init();
    cpu_halt_for_debug();
  }
  
  if(prefs.diskimage) {
    floppy_init(prefs.diskimage);
  } else {
    floppy_init("");
  }

  if(prefs.stateimage) {
    state = state_load(prefs.stateimage);
  }

  if(state != NULL)
    state_restore(state);

  while(cpu_run(CPU_RUN));
  return 0;
}
