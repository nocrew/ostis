#include "common.h"
#include "mmu.h"
#include "mfp.h"
#include "ikbd.h"
#include "diag.h"

#define ACIASIZE 4
#define ACIABASE 0xfffc00

#define ACIA_RX_FULL     0x01
#define ACIA_TX_EMPTY    0x02
#define ACIA_RX_OVERRUN  0x20
#define ACIA_IRQ         0x80
#define ACIA_IE          0x80

static BYTE acia_control;
static BYTE acia_status;
static BYTE rx_data;

HANDLE_DIAGNOSTICS(acia)

int acia_rx_full()
{
  return acia_status & ACIA_RX_FULL;
}

static void set_status(BYTE data)
{
  acia_status = data;
  if((acia_control & ACIA_IE) && (acia_status & ACIA_IRQ))
    mfp_clr_GPIP(MFP_GPIP_ACIA);
  else
    mfp_set_GPIP(MFP_GPIP_ACIA);
}

void acia_rx_data(BYTE data)
{
  TRACE("Rx %02x", data);
  rx_data = data;
  set_status(acia_status | ACIA_RX_FULL | ACIA_IRQ);
}

static void acia_reset(void)
{
  acia_control = 0;
  rx_data = 0;
  set_status(ACIA_TX_EMPTY);
}

static BYTE acia_read_byte(LONG addr)
{
  BYTE data;
  cpu_add_extra_cycles(6);

  switch(addr) {
  case 0xfffc00:
    return acia_status;
  case 0xfffc02:
    set_status(acia_status & ~(ACIA_RX_FULL | ACIA_IRQ));
    data = rx_data;
    TRACE("Read %02x", data);
    return data;
  default:
    return 0;
  }
}

static WORD acia_read_word(LONG addr)
{
  return (acia_read_byte(addr)<<8)|acia_read_byte(addr+1);
}

static LONG acia_read_long(LONG addr)
{
  return ((acia_read_byte(addr)<<24)|
	  (acia_read_byte(addr+1)<<16)|
	  (acia_read_byte(addr+2)<<8)|
	  (acia_read_byte(addr+3)));
}

static void acia_write_byte(LONG addr, BYTE data)
{
  cpu_add_extra_cycles(6);

  switch(addr) {
  case 0xfffc00:
    DEBUG("Control %02x", data);
    if(data == 3) {
      DEBUG("Reset");
      acia_reset();
    } else if((data & 0x7C) != 0x14)
      DEBUG("Unsupported configuration");
    acia_control = data;
    break;
  case 0xfffc02:
    TRACE("Write %02x", data);
    ikbd_write_byte(data);
    break;
  }
}

static void acia_write_word(LONG addr, WORD data)
{
  acia_write_byte(addr, (data&0xff00)>>8);
  acia_write_byte(addr+1, (data&0xff));
}

static void acia_write_long(LONG addr, LONG data)
{
  acia_write_byte(addr, (data&0xff000000)>>24);
  acia_write_byte(addr+1, (data&0xff0000)>>16);
  acia_write_byte(addr+2, (data&0xff00)>>8);
  acia_write_byte(addr+3, (data&0xff));
}

void acia_init()
{
  struct mmu *acia;

  acia = mmu_create("ACIA", "ACIA");
  if(!acia) {
    return;
  }
  acia->start = ACIABASE;
  acia->size = ACIASIZE;
  acia->read_byte = acia_read_byte;
  acia->read_word = acia_read_word;
  acia->read_long = acia_read_long;
  acia->write_byte = acia_write_byte;
  acia->write_word = acia_write_word;
  acia->write_long = acia_write_long;
  acia->diagnostics = acia_diagnostics;

  mmu_register(acia);
  acia_reset();
}
