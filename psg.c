#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"

#define PSGSIZE 256
#define PSGBASE 0xff8800

static BYTE psgreg[16];
static int psgactive = 0;

static BYTE psg_read_byte(LONG addr)
{
  addr &= 2;
  if(addr) return 0;
  return psgreg[psgactive];
}

static WORD psg_read_word(LONG addr)
{
  return (psg_read_byte(addr)<<8)|psg_read_byte(addr+1);
}

static LONG psg_read_long(LONG addr)
{
  return ((psg_read_byte(addr)<<24)|
       (psg_read_byte(addr+1)<<16)|
       (psg_read_byte(addr+2)<<8)|
       (psg_read_byte(addr+3)));
}

static void psg_write_byte(LONG addr, BYTE data)
{
  addr &= 2;

  switch(addr) {
  case 0:
    psgactive = data&0xf;
    break;
  case 2:
    psgreg[psgactive] = data;
    break;
  }
}

static void psg_write_word(LONG addr, WORD data)
{
  psg_write_byte(addr, (data&0xff00)>>8);
  psg_write_byte(addr+1, (data&0xff));
}

static void psg_write_long(LONG addr, LONG data)
{
  psg_write_byte(addr, (data&0xff000000)>>24);
  psg_write_byte(addr+1, (data&0xff0000)>>16);
  psg_write_byte(addr+2, (data&0xff00)>>8);
  psg_write_byte(addr+3, (data&0xff));
}

void psg_init()
{
  struct mmu *psg;

  psg = (struct mmu *)malloc(sizeof(struct mmu));
  if(!psg) {
    return;
  }
  psg->start = PSGBASE;
  psg->size = PSGSIZE;
  psg->name = strdup("PSG");
  psg->read_byte = psg_read_byte;
  psg->read_word = psg_read_word;
  psg->read_long = psg_read_long;
  psg->write_byte = psg_write_byte;
  psg->write_word = psg_write_word;
  psg->write_long = psg_write_long;

  mmu_register(psg);
}

void psg_print_status()
{
  int i;
  printf("PSG: %d: ", psgactive);
  for(i=0;i<16;i++)
    printf("%02x ",psgreg[i]);
  printf("\n\n");
}
