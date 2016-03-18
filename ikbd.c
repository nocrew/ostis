#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL_mouse.h>
#include "common.h"
#include "event.h"
#include "mmu.h"
#include "mfp.h"
#include "acia.h"
#include "state.h"
#include "diag.h"

#define IKBDFIFO 32
#define IKBD_INTINTERVAL 10000

static BYTE ikbd_fifo[IKBDFIFO];
static int ikbd_fifocnt = 0;
static uint64_t ikbd_next_interrupt_cycle = 0;
static BYTE ikbd_buttons = 0;
static BYTE ikbd_mouse_enabled = 0;
static int ikbd_mouse_inverted = 0;
static int ikbd_absolute_x = 0;
static int ikbd_absolute_y = 0;
static BYTE ikbd_joystick_enabled = 0;
static BYTE ikbd_cmdbuf[6];
static BYTE ikbd_cmdcnt = 0;
static void (*ikbd_cmdfn)(void);

static void ikbd_queue_fifo(BYTE data);
static void ikbd_do_interrupts(struct cpu *cpu);

HANDLE_DIAGNOSTICS(ikbd)

void ikbd_print_status()
{
  int i;
  printf("IKBD:\n");
  printf("Next interrupt cycle: %lld\n", (long long)ikbd_next_interrupt_cycle);
  printf("FIFO count: %d\n", ikbd_fifocnt);
  printf("FIFO:\n");
  for(i=0;i<ikbd_fifocnt;i++) {
    printf("%02x ", ikbd_fifo[i]);
    if(i%16 == 15) {
      printf("\n");
    }
  }
  printf("\n");
}

static void ikbd_set_mouse_button_action(void)
{
  DEBUG("Set mouse button action %02x", ikbd_cmdbuf[0]);
}

static void ikbd_set_absolute_mouse_positioning(void)
{
  ikbd_absolute_x = (ikbd_cmdbuf[3] << 8) + ikbd_cmdbuf[2];
  ikbd_absolute_y = (ikbd_cmdbuf[1] << 8) + ikbd_cmdbuf[0];
  ikbd_mouse_enabled = 0;
  DEBUG("Absolute max %d %d", ikbd_absolute_x, ikbd_absolute_y);
}

static void ikbd_set_mouse_scale(void)
{
  DEBUG("Set mouse scale %u %u", ikbd_cmdbuf[0], ikbd_cmdbuf[1]);
}

static void ikbd_mouse_report(void)
{
  int x, y, b, but = 0;
  b = SDL_GetMouseState(&x, &y);
  if (b & 1)
    but |= 4;
  else
    but |= 8;
  if (b & 4)
    but |= 1;
  else
    but |= 2;
  ikbd_queue_fifo(0xf7);
  ikbd_queue_fifo(but);
  x = (int)(((float)x * ikbd_absolute_x) / 1024.0);
  y = (int)(((float)y * ikbd_absolute_y) / 628.0);
  TRACE("Mouse at %d %d", x, y);
  ikbd_queue_fifo(x >> 8);
  ikbd_queue_fifo(x & 0xff);
  ikbd_queue_fifo(y >> 8);
  ikbd_queue_fifo(y & 0xff);
}

static void ikbd_load_mouse_position(void)
{
  WARN("Load mouse position %d %d, not handled", ikbd_cmdbuf[0], ikbd_cmdbuf[1]);
}

static void ikbd_set_mouse_threshold(void)
{
  WARN("Set mouse threshold %u %u, not handled", ikbd_cmdbuf[0], ikbd_cmdbuf[1]);
}

static void ikbd_return_clock(void)
{
  time_t now;
  struct tm clock;
  int year,month,day,hour,min,sec;
  int bcd_year,bcd_month,bcd_day,bcd_hour,bcd_min,bcd_sec;

  time(&now);
  localtime_r(&now, &clock);
  year = clock.tm_year + 1900;
  month = clock.tm_mon + 1;
  day = clock.tm_mday;
  hour = clock.tm_hour;
  min = clock.tm_min;
  sec = clock.tm_sec;

  bcd_year = (((year/10)%10)<<4)|year%10;
  bcd_month = (((month/10)%10)<<4)|month%10;
  bcd_day = (((day/10)%10)<<4)|day%10;
  bcd_hour = (((hour/10)%10)<<4)|hour%10;
  bcd_min = (((min/10)%10)<<4)|min%10;
  bcd_sec = (((sec/10)%10)<<4)|sec%10;
  
  ikbd_queue_fifo(0xfc);
  ikbd_queue_fifo(bcd_year);
  ikbd_queue_fifo(bcd_month);
  ikbd_queue_fifo(bcd_day);
  ikbd_queue_fifo(bcd_hour);
  ikbd_queue_fifo(bcd_min);
  ikbd_queue_fifo(bcd_sec);
}

static void ikbd_do_reset(void)
{
  ikbd_buttons = 0;
  ikbd_mouse_enabled = 1;
  ikbd_mouse_inverted = 0;
  ikbd_joystick_enabled = 0;
  ikbd_fifocnt = 0;
  ikbd_cmdcnt = 0;
}

static void ikbd_reset(void)
{
  DEBUG("Reset %02x", ikbd_cmdbuf[0]);
  ikbd_do_reset();
  if(ikbd_cmdbuf[0] == 0x01) {
    ikbd_queue_fifo(0xf1);
  }
}

/* Not implemented */
static void ikbd_set_clock(void)
{
  /* Just ignore this, but run it to consume the bytes that were sent */
}

static void ikbd_set_cmd(BYTE cmd)
{
  switch(cmd) {
  case 0x07:
    ikbd_cmdfn = ikbd_set_mouse_button_action;
    ikbd_cmdcnt = 1;
    break;
  case 0x08:
    DEBUG("Enable mouse relative");
    ikbd_mouse_enabled = 1;
    break;
  case 0x09:
    ikbd_cmdfn = ikbd_set_absolute_mouse_positioning;
    ikbd_cmdcnt = 4;
    break;
  case 0x0b:
    ikbd_cmdfn = ikbd_set_mouse_threshold;
    ikbd_cmdcnt = 2;
    break;
  case 0x0c:
    ikbd_cmdfn = ikbd_set_mouse_scale;
    ikbd_cmdcnt = 2;
    break;
  case 0x0d:
    ikbd_mouse_report();
    break;
  case 0x0e:
    ikbd_cmdfn = ikbd_load_mouse_position;
    ikbd_cmdcnt = 5;
    break;
  case 0x0f:
    ikbd_mouse_inverted = 1;
    break;
  case 0x10:
    ikbd_mouse_inverted = 0;
    break;
  case 0x12:
    DEBUG("Disable mouse");
    ikbd_mouse_enabled = 0;
    break;
  case 0x14:
    DEBUG("Joystick event reporting");
    ikbd_joystick_enabled = 1;
    break;
  case 0x1a:
    DEBUG("Disable joysticks");
    ikbd_joystick_enabled = 0;
    break;
  case 0x1b:
    WARN("Set clock (not handled)");
    ikbd_cmdfn = ikbd_set_clock;
    ikbd_cmdcnt = 6;
    break;
  case 0x1c:
    ikbd_return_clock();
    break;
  case 0x80:
    ikbd_cmdfn = ikbd_reset;
    ikbd_cmdcnt = 1;
    break;
  default:
    ERROR("UNHANDLED command 0x%02x", cmd);
    break;
  }
}

static int ikbd_pop_fifo()
{
  BYTE tmp;

  if(ikbd_fifocnt == 0) return -1;
  tmp = ikbd_fifo[0];
  memmove(&ikbd_fifo[0], &ikbd_fifo[1], IKBDFIFO-1);
  ikbd_fifocnt--;
  return tmp;
}

static void ikbd_poll(void)
{
  if(ikbd_fifocnt > 0 && !acia_rx_full()) {
    acia_rx_data(ikbd_pop_fifo());
  }
}

static void ikbd_queue_fifo(BYTE data)
{
  if(ikbd_fifocnt == IKBDFIFO) {
    DEBUG("Overrun");
  } else {
    ikbd_fifo[ikbd_fifocnt] = data;
    ikbd_fifocnt++;
  }
}

void ikbd_write_byte(BYTE data)
{
  if(ikbd_cmdcnt == 0) {
    ikbd_set_cmd(data);
  } else {
    ikbd_cmdbuf[--ikbd_cmdcnt] = data;
    if(ikbd_cmdcnt == 0)
      ikbd_cmdfn();
  }
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
   * ikbd_mouse_enabled   == 1
   * cmdbuf               == cmdbuf size
   * cmdcnt               == 1
   * cmdfn                == function pointer size
   */

  state->size = 20+IKBDFIFO+sizeof ikbd_cmdbuf+sizeof(void (*)(void));
  state->data = malloc(state->size);
  if(state->data == NULL)
    return STATE_INVALID;
  state_write_mem_long(&state->data[0], ikbd_fifocnt);
  //  state_write_mem_long(&state->data[4], ikbd_intcnt);
  //  state_write_mem_long(&state->data[8], lastcpucnt);
  state_write_mem_long(&state->data[12], IKBDFIFO);
  for(r=0;r<IKBDFIFO;r++) {
    state_write_mem_byte(&state->data[16+r], ikbd_fifo[r]);
  }
  state_write_mem_byte(&state->data[16+IKBDFIFO+2], ikbd_mouse_enabled);
  state_write_mem_byte(&state->data[16+IKBDFIFO+3], ikbd_cmdcnt);
  for(r=0;r<sizeof ikbd_cmdbuf;r++) {
    state_write_mem_byte(&state->data[16+IKBDFIFO+4+r], ikbd_cmdbuf[r]);
  }
  state_write_mem_ptr(&state->data[16+IKBDFIFO+4+sizeof ikbd_cmdbuf], ikbd_cmdfn);
  return STATE_VALID;
}

static void ikbd_state_restore(struct mmu_state *state)
{
  int r;
  int fifosize;

  ikbd_fifocnt = state_read_mem_long(&state->data[0]);
  //  ikbd_intcnt = state_read_mem_long(&state->data[4]);
  //  lastcpucnt = state_read_mem_long(&state->data[8]);
  fifosize = state_read_mem_long(&state->data[12]);
  for(r=0;r<fifosize;r++) {
    ikbd_fifo[r] = state_read_mem_byte(&state->data[16+r]);
  }
  ikbd_mouse_enabled = state_read_mem_byte(&state->data[16+IKBDFIFO+3]);
  ikbd_cmdcnt = state_read_mem_byte(&state->data[16+IKBDFIFO+3]);
  for(r=0;r<sizeof ikbd_cmdbuf;r++) {
    ikbd_cmdbuf[r] = state_read_mem_byte(&state->data[16+IKBDFIFO+4]);
  }
  ikbd_cmdfn = state_read_mem_ptr(&state->data[16+IKBDFIFO+4+sizeof ikbd_cmdbuf]);
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
  TRACE("Mouse moved %d %d", x, y);
  ikbd_queue_fifo(0xF8 | ikbd_buttons);
  ikbd_queue_fifo(x & 0xFF);
  if(ikbd_mouse_inverted)
    y = -y;
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

void ikbd_joystick(int direction)
{
  if(!ikbd_joystick_enabled) return;
  ikbd_queue_fifo(0xff);
  ikbd_queue_fifo(direction);
}

void ikbd_fire(int state)
{
  if(!ikbd_joystick_enabled) return;
  ikbd_queue_fifo(0xff);
  ikbd_queue_fifo(state << 7);
}

void ikbd_init()
{
  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(ikbd, "IKBD");
}

void ikbd_do_interrupt(struct cpu *cpu)
{
  if(cpu->cycle > ikbd_next_interrupt_cycle) {
    ikbd_poll();
    ikbd_next_interrupt_cycle = cpu->cycle + IKBD_INTINTERVAL;
  }
}
