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
#include "state.h"

int main(int argc, char *argv[])
{
  int i;
  struct state *state;

  mmu_init(); /* Must run before hardware module inits */
  ram_init();
  rom_init();
  cpu_init();
  cartridge_init();
  psg_init();
  midi_init();
  ikbd_init();
  rtc_init();
  fdc_init();
  mfp_init();
  screen_disable(0);
  screen_init();
  shifter_init();
#if DEBUG
  debug_init();
#endif

  if(argc > 1) {
    state = state_load(argv[1]);
    if(state == NULL) {
      floppy_init(argv[1]);
    } else {
      if(argc > 2) {
	floppy_init(argv[2]);
      } else {
	floppy_init("");
      }
    }
  } else {
    floppy_init("");
  }

  if(state != NULL)
    state_restore(state);
#if DEBUG
  while(!debug_event());
  return 0;
#else
  while(cpu_run());
#endif

  mmu_print_map();
  // cpu_print_status();
  //  cpu_add_debugpoint(0xfc0280);
  // cpu_add_debugpoint(0xfc4dc2);
  //  cpu_add_debugpoint(0xfc2dca);
  //  cpu_add_debugpoint(0xfc0020);
  //  cpu_add_debugpoint(0xe00030);
  for(i=0;i<100000000;i++) {
    cpu_step_instr(CPU_TRACE);
    // cpu_print_status();
    // mfp_print_status();
  }
  
  while(1) {
    screen_swap();
  }
  
  return 0;
}
