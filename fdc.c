#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"

#define FDCSIZE 16
#define FDCBASE 0xff8600

static WORD fdc_control;
static BYTE fdc_status;

static BYTE fdc_reg[2];
static int fdc_reg_active;
static LONG fdc_dmaaddr;

static BYTE fdc_read_byte(LONG addr)
{
  switch(addr) {
  case 0xff8604:
    return 0xff;
  case 0xff8605:
    return 0xff;
    return fdc_reg[fdc_reg_active];
  case 0xff8606:
    return 0;
  case 0xff8607:
    return fdc_status;
  case 0xff8609:
    return (fdc_dmaaddr&0xff0000)>>16;
  case 0xff860b:
    return (fdc_dmaaddr&0xff00)>>8;
  case 0xff860d:
    return fdc_dmaaddr&0xff;
  default:
    return 0;
  }
}

static WORD fdc_read_word(LONG addr)
{
  return (fdc_read_byte(addr)<<8)|fdc_read_byte(addr+1);
}

static LONG fdc_read_long(LONG addr)
{
  return ((fdc_read_byte(addr)<<24)|
	  (fdc_read_byte(addr+1)<<16)|
	  (fdc_read_byte(addr+2)<<8)|
	  (fdc_read_byte(addr+3)));
}

static void fdc_write_byte(LONG addr, BYTE data)
{
  switch(addr) {
  case 0xff8605:
    fdc_reg[fdc_reg_active] = data;
    break;
  case 0xff8606:
    fdc_control = (fdc_control&0xff)|(data<<8);
    break;
  case 0xff8607:
    fdc_control = (fdc_control&0xff00)|data;
    if(fdc_control&0x10) {
      fdc_reg_active = 0;
    } else {
      fdc_reg_active = 1;
    }
    break;
  case 0xff8609:
    fdc_dmaaddr = (fdc_dmaaddr&0xffff)|(data<<16);
    break;
  case 0xff860b:
    fdc_dmaaddr = (fdc_dmaaddr&0xff00ff)|(data<<8);
    break;
  case 0xff860d:
    fdc_dmaaddr = (fdc_dmaaddr&0xffff00)|data;
    break;
  }
}

static void fdc_write_word(LONG addr, WORD data)
{
  fdc_write_byte(addr, (data&0xff00)>>8);
  fdc_write_byte(addr+1, (data&0xff));
}

static void fdc_write_long(LONG addr, LONG data)
{
  fdc_write_byte(addr, (data&0xff000000)>>24);
  fdc_write_byte(addr+1, (data&0xff0000)>>16);
  fdc_write_byte(addr+2, (data&0xff00)>>8);
  fdc_write_byte(addr+3, (data&0xff));
}

void fdc_init()
{
  struct mmu *fdc;

  fdc = (struct mmu *)malloc(sizeof(struct mmu));
  if(!fdc) {
    return;
  }
  fdc->start = FDCBASE;
  fdc->size = FDCSIZE;
  fdc->name = strdup("FDC");
  fdc->read_byte = fdc_read_byte;
  fdc->read_word = fdc_read_word;
  fdc->read_long = fdc_read_long;
  fdc->write_byte = fdc_write_byte;
  fdc->write_word = fdc_write_word;
  fdc->write_long = fdc_write_long;

  mmu_register(fdc);
}
