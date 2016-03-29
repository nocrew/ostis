#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include "common.h"
#include "mmu.h"
#include "mmu_fallback.h"
#include "ram.h"
#include "rom.h"
#include "cpu.h"
#include "psg.h"
#include "midi.h"
#include "acia.h"
#include "ikbd.h"
#include "diag.h"
#if INCLUDE_RTC
#include "rtc.h"
#endif
#include "dma.h"
#include "fdc.h"
#include "hdc.h"
#include "mfp.h"
#include "prefs.h"
#include "shifter.h"
#include "glue.h"
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
int play_audio = 0;
int audio_device = 0;
int monitor_sm124 = 0;
int crop_screen = 0;
int verbosity = 3;
int clocked_cpu = 0;

#if TEST_BUILD
int test_mode = 0;
char *test_case_name;
#define OPT_TEST_MODE            99999
#endif


#define OPT_CART                 10000
#define OPT_FORCE_EXTREME_DISASM 10001
#define OPT_CROP_SCREEN          10002
#define OPT_LOGLEVELS            10003
#define OPT_AUDIO_DEVICE         10004

struct sigaction reset;

static void reset_action(int sig, siginfo_t *info, void *x)
{
  cpu_reset_in();
}

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
      {"crop-screen",           no_argument,       0, OPT_CROP_SCREEN },
      {"loglevels" ,            required_argument, 0, OPT_LOGLEVELS },
      {"audio-device" ,         required_argument, 0, OPT_AUDIO_DEVICE },
#if TEST_BUILD
      {"test-case",             required_argument, 0, OPT_TEST_MODE},
#endif
      {0,                       0,                 0, 0 }
    };
    c = getopt_long(argc, argv, "a:b:c:t:s:hdpyVAMvqK", long_options, &option_index);
    if(c == -1) break;

    switch(c) {
    case 'a':
      prefs_set("diskimage", optarg);
      break;
    case 'b':
      prefs_set("diskimage2", optarg);
      break;
    case 'c':
      prefs_set("hdimage", optarg);
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
    case OPT_CROP_SCREEN:
      crop_screen = 1;
      break;
    case OPT_LOGLEVELS:
      diag_set_module_levels(optarg);
      break;
    case OPT_AUDIO_DEVICE:
      if(!strncmp("list", optarg, 4)) {
        audio_device = -1;
      } else {
        audio_device = atoi(optarg);
      }
      break;
#if TEST_BUILD
    case OPT_TEST_MODE:
      test_case_name = xstrdup(optarg);
      test_mode = 1;
      break;
#endif
    case 'K':
      clocked_cpu = 1;
      break;
    case 'd':
      debugger = 1;
      break;
    case 'p':
      ppmoutput = 1;
      break;
    case 'y':
      psgoutput = 1;
      break;
    case 'v':
      verbosity++;
      break;
    case 'q':
      verbosity = MAX(1, verbosity-1);
      break;
    case 'V':
      screen_set_delay(20000);
      break;
    case 'A':
      play_audio = 1;
      break;
    case 'M':
      monitor_sm124 = 1;
      break;
    case 'h':
    default:
      printf("Usage: %s [-AdMpqvVy] [-a diskimage1] [-b diskimage2] [-c hdimage] [-t tosimage] [-s stateimage]\n",
	     argv[0]);
      exit(-1);
      break;
    }
  }

  /* Do not crop screen while debugging */
  if(debugger) {
    crop_screen = 0;
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
  if(clocked_cpu) {
    cpu_init_clocked();
  } else {
    cpu_init();
  }
  if(prefs.cartimage) {
    cartridge_init(prefs.cartimage);
  } else {
    cartridge_init(NULL);
  }
  psg_init();
  midi_init();
  acia_init();
  ikbd_init();
#if INCLUDE_RTC
  rtc_init();
#endif
  dma_init();
  fdc_init();
  hdc_init(prefs.hdimage);
  mfp_init();
  screen_disable(0);
  glue_init();
  shifter_init();
  if(debugger) {
    debug_init();
    cpu_halt_for_debug();
  }
  screen_init();
  
  floppy_init(prefs.diskimage, prefs.diskimage2);

  if(prefs.stateimage) {
    state = state_load(prefs.stateimage);
  }

  if(state != NULL)
    state_restore(state);

  memset(&reset, 0, sizeof reset);
  reset.sa_sigaction = reset_action;
  sigaction(SIGHUP, &reset, NULL);

  if(clocked_cpu) {
    while(cpu_run_clocked(CPU_RUN));
  } else {
    while(cpu_run(CPU_RUN));
  }
  
  return 0;
}

void *xmalloc(size_t size)
{
  void *result = calloc(size, 1);
  if(result == NULL) {
    fprintf(stderr, "Unable to allocate %lld bytes", (long long)size);
    exit(1);
  }
  return result;
}

char *xstrdup(char *s)
{
  char *result = strdup(s);
  if(result == NULL) {
    fprintf(stderr, "Call to xstrdup failed");
    exit(1);
  }
  return result;
}
