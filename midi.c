#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"
#include "state.h"

#define MIDISIZE 4
#define MIDIBASE 0xfffc04

static BYTE midi_control;
static BYTE midi_status;
static BYTE midi_data;

static BYTE midi_read_byte(LONG addr)
{
  switch(addr) {
  case 0xfffc04:
    return midi_status;
  case 0xfffc06:
    return midi_data;
  default:
    return 0;
  }
}

static WORD midi_read_word(LONG addr)
{
  return (midi_read_byte(addr)<<8)|midi_read_byte(addr+1);
}

static LONG midi_read_long(LONG addr)
{
  return ((midi_read_byte(addr)<<24)|
	  (midi_read_byte(addr+1)<<16)|
	  (midi_read_byte(addr+2)<<8)|
	  (midi_read_byte(addr+3)));
}

static void midi_write_byte(LONG addr, BYTE data)
{
  switch(addr) {
  case 0xfffc04:
    midi_control = data;
    break;
  case 0xfffc06:
    midi_data = data;
    break;
  }
}

static void midi_write_word(LONG addr, WORD data)
{
  midi_write_byte(addr, (data&0xff00)>>8);
  midi_write_byte(addr+1, (data&0xff));
}

static void midi_write_long(LONG addr, LONG data)
{
  midi_write_byte(addr, (data&0xff000000)>>24);
  midi_write_byte(addr+1, (data&0xff0000)>>16);
  midi_write_byte(addr+2, (data&0xff00)>>8);
  midi_write_byte(addr+3, (data&0xff));
}

static int midi_state_collect(struct mmu_state *state)
{
  state->size = 0;
  return STATE_VALID;
}

static void midi_state_restore(struct mmu_state *state)
{
}

void midi_init()
{
  struct mmu *midi;

  midi = (struct mmu *)malloc(sizeof(struct mmu));
  if(!midi) {
    return;
  }
  midi->start = MIDIBASE;
  midi->size = MIDISIZE;
  midi->name = strdup("MIDI");
  strcpy(midi->id, "MIDI");
  midi->read_byte = midi_read_byte;
  midi->read_word = midi_read_word;
  midi->read_long = midi_read_long;
  midi->write_byte = midi_write_byte;
  midi->write_word = midi_write_word;
  midi->write_long = midi_write_long;
  midi->state_collect = midi_state_collect;
  midi->state_restore = midi_state_restore;

  mmu_register(midi);
}
