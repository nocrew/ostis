#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "event.h"
#include "mmu.h"
#include "mfp.h"

#define IKBDSIZE 4
#define IKBDBASE 0xfffc00

#define IKBDFIFO 3

static BYTE ikbd_control;
static BYTE ikbd_status = 0x2; /* Always ready */
static BYTE ikbd_writereg = 0;
static BYTE ikbd_fifo[IKBDFIFO];
static int ikbd_fifocnt = 0;

static int ikbd_pop_fifo()
{
  BYTE tmp;

  if(ikbd_fifocnt == 0) return 0;
  tmp = ikbd_fifo[0];
  ikbd_fifo[0] = ikbd_fifo[1];
  ikbd_fifo[1] = ikbd_fifo[2];
  ikbd_fifocnt--;
  if(ikbd_fifocnt > 0)
    ikbd_status |= 0x1;
  return tmp;
}

static void ikbd_queue_fifo(BYTE data)
{
  if(ikbd_fifocnt == IKBDFIFO) {
    ikbd_pop_fifo();
    ikbd_status |= 0x20;
    if(ikbd_control&0x80) {
      ikbd_status |= 0x80;
      mfp_set_GPIP(4);
      mfp_do_interrupt(cpu, 6);
    }
  }
  ikbd_fifo[ikbd_fifocnt] = data;
  ikbd_fifocnt++;
  ikbd_status |= 0x1;
}

static BYTE ikbd_read_byte(LONG addr)
{
  BYTE tmp;

  switch(addr) {
  case 0xfffc00:
    return ikbd_status;
  case 0xfffc02:
    ikbd_status &= ~0xa1;
    tmp = ikbd_pop_fifo();
    return tmp;
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
    break;
  case 0xfffc02:
    ikbd_status &= ~0x80;
    ikbd_writereg = data;
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

void ikbd_queue_key(int key, int state)
{
  if(state == EVENT_PRESS) {
    ikbd_queue_fifo(key);
  } else {
    ikbd_queue_fifo(key+0x80);
  }
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
  ikbd->name = strdup("IKBD");
  ikbd->read_byte = ikbd_read_byte;
  ikbd->read_word = ikbd_read_word;
  ikbd->read_long = ikbd_read_long;
  ikbd->write_byte = ikbd_write_byte;
  ikbd->write_word = ikbd_write_word;
  ikbd->write_long = ikbd_write_long;

  mmu_register(ikbd);
}

void ikbd_do_interrupts()
{
  if(ikbd_control&0x80)
    if(ikbd_fifocnt > 0) {
      if(IPL < 6) {
	ikbd_status |= 0x80;
	mfp_set_GPIP(4);
	mfp_do_interrupt(cpu, 6);
      }
    }
}
