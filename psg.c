#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "common.h"
#include "floppy.h"
#include "mmu.h"

#define PSGSIZE 256
#define PSGBASE 0xff8800


#define PSG_APERL  0
#define PSG_APERH  1
#define PSG_BPERL  2
#define PSG_BPERH  3
#define PSG_CPERL  4
#define PSG_CPERH  5
#define PSG_NOISE  6
#define PSG_MIXER  7
#define PSG_AVOL   8
#define PSG_BVOL   9
#define PSG_CVOL  10
#define PSG_EPERL 11
#define PSG_EPERH 12
#define PSG_ENV   13
#define PSG_IOA   14
#define PSG_IOB   15

#define PSG_CHA 0
#define PSG_CHB 1
#define PSG_CHC 2

/* Floppy uses IO port A */
#define PSG_IOA_SIDE (psgreg[PSG_IOA]&0x1)
#define PSG_IOA_DEV0 (psgreg[PSG_IOA]&0x2)
#define PSG_IOA_DEV1 (psgreg[PSG_IOA]&0x4)


#define PSG_BASEFREQ 2000000
#define PSG_PERIODDIV   16.0

/* Presample is a 2MS/s data stream for use to build the sample in
   output sample rate */
#define PSG_PRESAMPLESIZE 4096
#define PSG_OUTPUT_FREQ 44100
#define PSG_PRESAMPLES_PER_SAMPLE (PSG_BASEFREQ/PSG_OUTPUT_FREQ)

static BYTE psgreg[16];
static int psgactive = 0;
static int psg_periodcnt[3];
static int psg_volume[3];
static int psg_noisecnt;
static int psg_noise;

/* Fixed point volume levels, voltage*100000 */
static int psg_volume_voltage[32] = {
  0, 611, 978, 1710, 2200, 3300, 4400, 6360, 
  8800, 12500, 17600, 25200, 35500, 50400, 70300, 100000,
  /* second half to secure that there are enough values */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static int psg_register_mask[16] = {
  0xff, 0xf, 0xff, 0xf, 0xff, 0xf, 0x1f, 0xff,
  0x1f, 0x1f, 0x1f, 0xff, 0xff, 0xf, 0xff, 0xff
};
static int psg_presample[PSG_PRESAMPLESIZE][3];
static int psg_presamplepos = 0; /* circular buffer */
static int psg_presamplestart = 0;
static long lastcpucnt = 0;
static int snd_fd;

static int psg_set_period(int channel)
{
  return PSG_PERIODDIV*
    (psgreg[PSG_APERL+channel*2]|(psgreg[PSG_APERH+channel*2]<<8));
}

static void psg_set_register(BYTE data)
{
  psgreg[psgactive] = data & psg_register_mask[psgactive];

  switch(psgactive) {
  case PSG_APERL:
  case PSG_APERH:
    psg_periodcnt[PSG_CHA] = psg_set_period(PSG_CHA);
    break;
  case PSG_BPERL:
  case PSG_BPERH:
    psg_periodcnt[PSG_CHB] = psg_set_period(PSG_CHB);
    break;
  case PSG_CPERL:
  case PSG_CPERH:
    psg_periodcnt[PSG_CHC] = psg_set_period(PSG_CHC);
    break;
  case PSG_NOISE:
    psg_noisecnt = PSG_PERIODDIV*psgreg[PSG_NOISE];
  case PSG_MIXER:
    break;
  case PSG_AVOL:
    psg_volume[PSG_CHA] = psg_volume_voltage[psgreg[psgactive]];
    break;
  case PSG_BVOL:
    psg_volume[PSG_CHB] = psg_volume_voltage[psgreg[psgactive]];
    break;
  case PSG_CVOL:
    psg_volume[PSG_CHC] = psg_volume_voltage[psgreg[psgactive]];
    break;
  case PSG_EPERL:
  case PSG_EPERH:
  case PSG_ENV:
    break;
  case PSG_IOA:
    floppy_side(PSG_IOA_SIDE);
    floppy_active((PSG_IOA_DEV0|PSG_IOA_DEV1)>>1);
    break;
  case PSG_IOB:
  default:
    break;
  }
}

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
    psg_set_register(data);
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

  snd_fd = open("psg.raw", O_WRONLY|O_TRUNC|O_CREAT, 0644);
}

void psg_print_status()
{
  int i;
  printf("PSG: %d: ", psgactive);
  for(i=0;i<16;i++)
    printf("%02x ",psgreg[i]);
  printf("\n\n");
}

static void psg_generate_presamples(long cycles)
{
  int i,c;
  int out;
  int noise;
  int tone[3];
  int tonemask, noisemask;

  for(i=0;i<cycles;i++) {
    for(c=0;c<3;c++) {
      psg_periodcnt[c]--;
      if(psg_periodcnt[c] < 0) {
	psg_periodcnt[c] = psg_set_period(c);
      }
      if(psg_periodcnt[c] > (psg_set_period(c)/2)) {
	tone[c] = 1;
      } else {
	tone[c] = 0;
      }
    }

    psg_noisecnt--;
    if(psg_noisecnt < 0) {
      psg_noisecnt = psgreg[PSG_NOISE];
      psg_noise |= ((psg_noise^(psg_noise>>2))&1)<<17;
    }
    noise = psg_noise&1;

    for(c=0;c<3;c++) {
      tonemask = (psgreg[PSG_MIXER]>>c)&1;
      noisemask = (psgreg[PSG_MIXER]>>(c+3))&1;
      out = (tone[c]|tonemask)&(noise|noisemask);
      psg_presample[(psg_presamplepos)%PSG_PRESAMPLESIZE][c] = 
	out * psg_volume[c];
    }
    psg_presamplepos++;
    if(psg_presamplepos > (PSG_PRESAMPLESIZE-1))
      psg_presamplepos = 0;
  }
}

static void psg_generate_samples()
{
  int presamples,pos;
  int i,c;
  long out;

  if(psg_presamplepos < psg_presamplestart) {
    pos = psg_presamplepos + PSG_PRESAMPLESIZE;
  } else {
    pos = psg_presamplepos;
  }
  
  presamples = pos - psg_presamplestart;

  //  printf("DEBUG: --- entering loop\n");
  while(presamples >= PSG_PRESAMPLES_PER_SAMPLE) {
    //    printf("DEBUG: presamples == %d (%d - %d - %d)\n", 
    //	   presamples, psg_presamplepos, psg_presamplestart, pos);
    out = 0;
    for(i=0;i<PSG_PRESAMPLES_PER_SAMPLE;i++) {
      for(c=0;c<3;c++) {
	out += psg_presample[(psg_presamplestart+i)%PSG_PRESAMPLESIZE][c];
      }
    }
    out = ((out/(PSG_PRESAMPLES_PER_SAMPLE*3))*65535)/100000;
    out |= out<<16;
    write(snd_fd, &out, 4);
    psg_presamplestart += PSG_PRESAMPLES_PER_SAMPLE;
    if(psg_presamplestart > (PSG_PRESAMPLESIZE-1)) {
      psg_presamplestart -= PSG_PRESAMPLESIZE;
    }
    if(psg_presamplepos < psg_presamplestart) {
      pos = psg_presamplepos + PSG_PRESAMPLESIZE;
    } else {
      pos = psg_presamplepos;
    }
    presamples = pos - psg_presamplestart;
  }
}

void psg_do_interrupts(struct cpu *cpu)
{
  long tmpcpu;
  
  tmpcpu = cpu->cycle - lastcpucnt;
  if(tmpcpu < 0) {
    tmpcpu += MAX_CYCLE;
  }

  psg_generate_presamples(tmpcpu/4);
  psg_generate_samples();

  lastcpucnt = cpu->cycle;
}
