#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "event.h"
#include "mmu.h"
#include "mfp.h"
#include "state.h"

#define IKBDSIZE 4
#define IKBDBASE 0xfffc00

#define IKBDFIFO 32
#define IKBD_INTCNT 7200

static BYTE ikbd_control;
static BYTE ikbd_status = 0x2; /* Always ready */
static BYTE ikbd_writereg = 0;
static BYTE ikbd_fifo[IKBDFIFO];
static int ikbd_fifocnt = 0;
static int ikbd_intcnt = 0;
static int lastcpucnt = 0;
static BYTE ikbd_buttons = 0;
static BYTE ikbd_waiting_for_cmd = 1;
static BYTE ikbd_mouse_enabled = 0;

static void ikbd_set_cmd(BYTE cmd)
{
  if(cmd == 0x08) {
    // Enable mouse relative
    ikbd_mouse_enabled = 1;
    ikbd_waiting_for_cmd = 1;
  } else if(cmd == 0x12) {
    // Disable mouse
    ikbd_mouse_enabled = 0;
    ikbd_waiting_for_cmd = 1;
  } else {
    // Unhandled ikbd commands (nothing for now)
    ikbd_waiting_for_cmd = 1;
  }
}

static int ikbd_pop_fifo()
{
  BYTE tmp;

  if(ikbd_fifocnt == 0) return -1;
  tmp = ikbd_fifo[0];
  memmove(&ikbd_fifo[0], &ikbd_fifo[1], IKBDFIFO-1);
  ikbd_fifocnt--;
  if(ikbd_fifocnt > 0)
    ikbd_status |= 0x81;
  return tmp;
}

static void ikbd_queue_fifo(BYTE data)
{
  if(ikbd_fifocnt == IKBDFIFO) {
    ikbd_pop_fifo();
    ikbd_status |= 0x20;
    if(ikbd_control&0x80) {
      ikbd_intcnt = IKBD_INTCNT;
      ikbd_status |= 0x80;
      mfp_clr_GPIP(4);
      //      mfp_do_interrupt(cpu, 6);
    }
  }
  ikbd_fifo[ikbd_fifocnt] = data;
  ikbd_fifocnt++;
  ikbd_status |= 0x81;
}

static BYTE ikbd_read_byte(LONG addr)
{
  static BYTE last = 0;
  int tmp;

  switch(addr) {
  case 0xfffc00:
    return ikbd_status;
  case 0xfffc02:
    ikbd_status &= ~0xa1;
    if(ikbd_control&0x80)
      ikbd_intcnt = IKBD_INTCNT;
    else
      ikbd_intcnt = 0;
    tmp = ikbd_pop_fifo();
    if(tmp == -1)
      return last;
    else
      return last = tmp;
  default:
    return 0;
  }
}

static WORD ikbd_read_word(LONG addr)
{
  return (ikbd_read_byte(addr)<<8)|ikbd_read_byte(addr+1);
}

static LONG ikbd_read_long(LONG addr)
{
  return ((ikbd_read_byte(addr)<<24)|
	  (ikbd_read_byte(addr+1)<<16)|
	  (ikbd_read_byte(addr+2)<<8)|
	  (ikbd_read_byte(addr+3)));
}

static void ikbd_write_byte(LONG addr, BYTE data)
{
  switch(addr) {
  case 0xfffc00:
    ikbd_control = data;
    if(data&0x80)
      ikbd_intcnt = IKBD_INTCNT;
    else
      ikbd_intcnt = 0;
    break;
  case 0xfffc02:
    ikbd_status &= ~0x80;
    mfp_set_GPIP(4);
    if(ikbd_waiting_for_cmd) {
      ikbd_set_cmd(data);
    }
    break;
  }
}

static void ikbd_write_word(LONG addr, WORD data)
{
  ikbd_write_byte(addr, (data&0xff00)>>8);
  ikbd_write_byte(addr+1, (data&0xff));
}

static void ikbd_write_long(LONG addr, LONG data)
{
  ikbd_write_byte(addr, (data&0xff000000)>>24);
  ikbd_write_byte(addr+1, (data&0xff0000)>>16);
  ikbd_write_byte(addr+2, (data&0xff00)>>8);
  ikbd_write_byte(addr+3, (data&0xff));
}

static int ikbd_state_collect(struct mmu_state *state)
{
  int r;

  /* Size:
   * 
   * ikbd_fifocnt         == 4
   * ikbd_intcnt          == 4
   * lastcpucnt           == 4
   * ikbd_fifosize        == 4
   * ikbd_fifo            == 1*fifosize
   * ikbd_control         == 1
   * ikbd_status          == 1
   * ikbd_writereg        == 1
   * ikbd_mouse_enabled   == 1
   * ikbd_waiting_for_cmd == 1
   */

  state->size = 21+IKBDFIFO;
  state->data = (char *)malloc(state->size);
  if(state->data == NULL)
    return STATE_INVALID;
  state_write_mem_long(&state->data[0], ikbd_fifocnt);
  state_write_mem_long(&state->data[4], ikbd_intcnt);
  state_write_mem_long(&state->data[8], lastcpucnt);
  state_write_mem_long(&state->data[12], IKBDFIFO);
  for(r=0;r<IKBDFIFO;r++) {
    state_write_mem_byte(&state->data[16+r], ikbd_fifo[r]);
  }
  state_write_mem_byte(&state->data[16+IKBDFIFO], ikbd_control);
  state_write_mem_byte(&state->data[16+IKBDFIFO+1], ikbd_status);
  state_write_mem_byte(&state->data[16+IKBDFIFO+2], ikbd_writereg);
  state_write_mem_byte(&state->data[16+IKBDFIFO+3], ikbd_mouse_enabled);
  state_write_mem_byte(&state->data[16+IKBDFIFO+4], ikbd_waiting_for_cmd);
  return STATE_VALID;
}

static void ikbd_state_restore(struct mmu_state *state)
{
  int r;
  int fifosize;

  ikbd_fifocnt = state_read_mem_long(&state->data[0]);
  ikbd_intcnt = state_read_mem_long(&state->data[4]);
  lastcpucnt = state_read_mem_long(&state->data[8]);
  fifosize = state_read_mem_long(&state->data[12]);
  for(r=0;r<fifosize;r++) {
    ikbd_fifo[r] = state_read_mem_byte(&state->data[16+r]);
  }
  ikbd_control = state_read_mem_byte(&state->data[16+IKBDFIFO]);
  ikbd_status = state_read_mem_byte(&state->data[16+IKBDFIFO+1]);
  ikbd_writereg = state_read_mem_byte(&state->data[16+IKBDFIFO+2]);
  ikbd_mouse_enabled = state_read_mem_byte(&state->data[16+IKBDFIFO+3]);
  ikbd_waiting_for_cmd = state_read_mem_byte(&state->data[16+IKBDFIFO+4]);
}

void ikbd_queue_key(int key, int state)
{
  if(state == EVENT_PRESS) {
    ikbd_queue_fifo(key);
  } else {
    ikbd_queue_fifo(key+0x80);
  }
}

void ikbd_queue_motion(int x, int y)
{
  if(!ikbd_mouse_enabled) return;
  ikbd_queue_fifo(0xF8 | ikbd_buttons);
  ikbd_queue_fifo(x & 0xFF);
  ikbd_queue_fifo(y & 0xFF);
}

void ikbd_button(int button, int state)
{
  if(!ikbd_mouse_enabled) return;
  if(state == EVENT_PRESS) {
    ikbd_buttons |= button;
  } else {
    ikbd_buttons &= ~button;
  }
  ikbd_queue_motion(0, 0);
}

void ikbd_init()
{
  struct mmu *ikbd;

  ikbd = (struct mmu *)malloc(sizeof(struct mmu));
  if(!ikbd) {
    return;
  }
  ikbd->start = IKBDBASE;
  ikbd->size = IKBDSIZE;
  memcpy(ikbd->id, "IKBD", 4);
  ikbd->name = strdup("IKBD");
  ikbd->read_byte = ikbd_read_byte;
  ikbd->read_word = ikbd_read_word;
  ikbd->read_long = ikbd_read_long;
  ikbd->write_byte = ikbd_write_byte;
  ikbd->write_word = ikbd_write_word;
  ikbd->write_long = ikbd_write_long;
  ikbd->state_collect = ikbd_state_collect;
  ikbd->state_restore = ikbd_state_restore;

  mmu_register(ikbd);
}

void ikbd_do_interrupts(struct cpu *cpu)
{
  long tmpcpu;
  tmpcpu = cpu->cycle - lastcpucnt;
  if(tmpcpu < 0) {
    tmpcpu += MAX_CYCLE;
  }

  if(ikbd_intcnt > 0) {
    ikbd_intcnt -= tmpcpu;
    if(cpu->debug) {
      if(ikbd_fifocnt) {
	printf("DEBUG: ikbd_intcnt == %d\n", ikbd_intcnt);
	printf("DEBUG: ikbd_control == %02x\n", ikbd_control);
	printf("DEBUG: ikbd_fifocnt == %d\n", ikbd_fifocnt);
	printf("DEBUG: ikbd_status == %02x\n", ikbd_status);
      }
    }
    if(ikbd_intcnt <= 0) {
      if(ikbd_control&0x80) {
	if(ikbd_fifocnt > 0) {
	  ikbd_status |= 0x81;
	  mfp_clr_GPIP(4);
	  //printf("DEBUG: sending keyboard interrupt\n");
	  mfp_set_pending(6);
	} else {
	  mfp_set_GPIP(4);
	}
      }
      ikbd_intcnt += IKBD_INTCNT;
      if(cpu->debug)
	printf("DEBUG: Resetting ikbd_intcnt\n");
    }
  }

  lastcpucnt = cpu->cycle;
}
