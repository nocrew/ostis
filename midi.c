/*
 * MC6805 ACIA	Connected to
 *
 * D0-D7	BUS:D0-D7
 * E		CPU:E
 * R/W		R/W
 * CS0		GLUE:6850CS
 * CS1		+5V
 * CS2		BUS:A2
 * RS		BUS:A1
 * IRQ		MFP:I4
 * TX CLK	0.5MHz
 * RX CLK	0.5MHz
 * TX DATA	MIDI OUT
 * RX DATA	MIDI IN
 * CTS		GND
 * RTS		-
 * DCD		GND
 */

#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"
#include "state.h"
#include "diag.h"

#define MIDISIZE 4
#define MIDIBASE 0xfffc04

static BYTE midi_control;
static BYTE midi_status = 0x02; /* Always ready to receive */
static BYTE midi_data;

HANDLE_DIAGNOSTICS(midi)

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

  midi = mmu_create("MIDI", "MIDI");
  midi->start = MIDIBASE;
  midi->size = MIDISIZE;
  midi->read_byte = midi_read_byte;
  midi->read_word = midi_read_word;
  midi->write_byte = midi_write_byte;
  midi->write_word = midi_write_word;
  midi->state_collect = midi_state_collect;
  midi->state_restore = midi_state_restore;
  midi->diagnostics = midi_diagnostics;

  mmu_register(midi);
}
