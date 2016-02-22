#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "common.h"
#include "floppy.h"
#include "mmu.h"
#include "state.h"
#include <SDL.h>

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

#define PSG_ENV_CONTINUE  (psgreg[PSG_ENV]&0x8)
#define PSG_ENV_ATTACK    (psgreg[PSG_ENV]&0x4)
#define PSG_ENV_ALTERNATE (psgreg[PSG_ENV]&0x2)
#define PSG_ENV_HOLD      (psgreg[PSG_ENV]&0x1)

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
static int psg_periodcnt[3] = { -1, -1, -1 };
static int psg_volume[3];
static int psg_noisecnt = -1;
static int psg_noise = 1;
static int env_attack = 0;
static int env_held = 0;
static int env_first = 1;
static int env_volume;
static int env_cnt = -1;

#if SDL_HAS_QUEUEAUDIO
static SDL_AudioDeviceID psg_audio_device;
static SDL_AudioSpec want,have;
#endif

#define VOLUME_OFFSET(x) ((x==0)?0:(1+x*2))

/* Fixed point volume levels, voltage*65535 */
/* 32 values because 2149 has 32 levels in envelope, and 16 in non-env */
static signed long psg_volume_voltage[32] = {
  0, 366, 435, 517, 615, 732, 870, 1035,
  1231, 1464, 1740, 2069, 2460, 2924, 3476, 4132, 
  4912, 5838, 6939, 8248, 9803, 11651, 13848, 16459, 
  19562, 23250, 27633, 32843, 39035, 46394, 55140, 65535
};

static int psg_register_mask[16] = {
  0xff, 0xf, 0xff, 0xf, 0xff, 0xf, 0x1f, 0xff,
  0x1f, 0x1f, 0x1f, 0xff, 0xff, 0xf, 0xff, 0xff
};
static signed long psg_presample[PSG_PRESAMPLESIZE][3];
static int psg_presamplepos = 0; /* circular buffer */
static int psg_presamplestart = 0;
static long lastcpucnt = 0;
static int snd_fd;
static int psg_running = 1;

static int psg_set_period(int channel)
{
  return PSG_PERIODDIV*
    (psgreg[PSG_APERL+channel*2]|(psgreg[PSG_APERH+channel*2]<<8));
}

static int env_set_period()
{
  return (PSG_PERIODDIV*16)*(psgreg[PSG_EPERL]|(psgreg[PSG_EPERH]<<8));
}

static void psg_set_register(BYTE data)
{
  psgreg[psgactive] = data & psg_register_mask[psgactive];

  switch(psgactive) {
  case PSG_APERL:
  case PSG_APERH:
    break;
  case PSG_BPERL:
  case PSG_BPERH:
    break;
  case PSG_CPERL:
  case PSG_CPERH:
    break;
  case PSG_NOISE:
  case PSG_MIXER:
    break;
  case PSG_AVOL:
    if(!(psgreg[psgactive]&0x10))
      psg_volume[PSG_CHA] = 
	psg_volume_voltage[VOLUME_OFFSET(psgreg[psgactive])];
    break;
  case PSG_BVOL:
    if(!(psgreg[psgactive]&0x10))
      psg_volume[PSG_CHB] = 
	psg_volume_voltage[VOLUME_OFFSET(psgreg[psgactive])];
    break;
  case PSG_CVOL:
    if(!(psgreg[psgactive]&0x10))
      psg_volume[PSG_CHC] = 
	psg_volume_voltage[VOLUME_OFFSET(psgreg[psgactive])];
    break;
  case PSG_EPERL:
  case PSG_EPERH:
    break;
  case PSG_ENV:
    env_first = 0;
    env_held = 0;
    env_attack = PSG_ENV_ATTACK;
    env_cnt = env_set_period();
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
  if(addr&1) return;
  addr &= 2;

  ADD_CYCLE(2);

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

static int psg_state_collect(struct mmu_state *state)
{
  state->size = 0;
  return STATE_VALID;
}

static void psg_state_restore(struct mmu_state *state)
{
}

void psg_init()
{
  int i;
  struct mmu *psg;

  psg = (struct mmu *)malloc(sizeof(struct mmu));
  if(!psg) {
    return;
  }
  psg->start = PSGBASE;
  psg->size = PSGSIZE;
  memcpy(psg->id, "PSG0", 4);
  psg->name = strdup("PSG");
  psg->read_byte = psg_read_byte;
  psg->read_word = psg_read_word;
  psg->read_long = psg_read_long;
  psg->write_byte = psg_write_byte;
  psg->write_word = psg_write_word;
  psg->write_long = psg_write_long;
  psg->state_collect = psg_state_collect;
  psg->state_restore = psg_state_restore;

  mmu_register(psg);

  if(psgoutput) {
    snd_fd = open("psg.raw", O_WRONLY|O_TRUNC|O_CREAT, 0644);
  }

#if SDL_HAS_QUEUEAUDIO
  if(play_audio) {
    int count = SDL_GetNumAudioDevices(0);
    SDL_memset(&want, 0, sizeof(want));
    want.freq = PSG_OUTPUT_FREQ;
    want.format = AUDIO_S16LSB;
    want.channels = 2;
    want.samples = 4096;
  
    printf("DEBUG: Audio devices: %d\n", count);
    for (i = 0; i < count; ++i) {
      SDL_Log("Audio device %d: %s\n", i, SDL_GetAudioDeviceName(i, 0));
      psg_audio_device = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(i, 0), 0,
                                             &want, &have, 0);
      break;
    }
  
  }
#endif
  for(i=0;i<32;i++) {
    psg_volume_voltage[i] /= 2;
  }
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
      psg_noisecnt = PSG_PERIODDIV*psgreg[PSG_NOISE];
      psg_noise |= ((psg_noise^(psg_noise>>2))&1)<<17;
      psg_noise >>= 1;
    }
    noise = psg_noise&1;

    if(env_set_period()) {
      env_cnt--;
      if(env_cnt < 0) {
	env_cnt = env_set_period();
	if(env_first) { /* Make sure initial -1 is ignored */
	  env_first = 0;
	} else {
	  if(!PSG_ENV_CONTINUE) {
	    env_volume = 0;
	    env_held = 1;
	  } else {
	    if(PSG_ENV_HOLD) {
	      env_held = 1;
	      if((psgreg[PSG_ENV] == 11) || (psgreg[PSG_ENV] == 13)) { 
		/* (11, 13) end high */
		env_volume = 31;
	      } else { /* (9, 15) should end low */
		env_volume = 0;
	      }
	    } else {
	      if(PSG_ENV_ALTERNATE) {
		if(env_attack) {
		  env_attack = 0;
		} else {
		  env_attack = 1;
		}
	      }
	    }
	  }
	}
      }
      if(!env_held) {
	int vol;
	vol = (31*env_cnt)/env_set_period();
	if(env_attack) {
	  env_volume = psg_volume_voltage[31-vol];
	} else {
	  env_volume = psg_volume_voltage[vol];
	}
      }
    }

    for(c=0;c<3;c++) {
      tonemask = (psgreg[PSG_MIXER]>>c)&1;
      noisemask = (psgreg[PSG_MIXER]>>(c+3))&1;
      out = (tone[c]|tonemask)&(noise|noisemask);
      out = (out<<1)-1;
      if(psgreg[PSG_AVOL+c]&0x10) {
	psg_presample[(psg_presamplepos)%PSG_PRESAMPLESIZE][c] =
	  out * env_volume;
      } else {
	psg_presample[(psg_presamplepos)%PSG_PRESAMPLESIZE][c] = 
	  out * psg_volume[c];
      }
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
  signed long out;
  signed short outsign;

  if(psg_presamplepos < psg_presamplestart) {
    pos = psg_presamplepos + PSG_PRESAMPLESIZE;
  } else {
    pos = psg_presamplepos;
  }
  
  presamples = pos - psg_presamplestart;

  while(presamples >= PSG_PRESAMPLES_PER_SAMPLE) {
    out = 0;
    for(i=0;i<PSG_PRESAMPLES_PER_SAMPLE;i++) {
      for(c=0;c<3;c++) {
	out += psg_presample[(psg_presamplestart+i)%PSG_PRESAMPLESIZE][c];
      }
    }
    out = (out/(PSG_PRESAMPLES_PER_SAMPLE*3));
    if(psgoutput) {
      outsign = out&0xffff;
    }

    if(!psg_running && (out != 0))
      psg_running = 1;
    if(psg_running) {
      if(psgoutput) {
        if(write(snd_fd, &outsign, 2) != 2)
          WARNING(write);
        if(write(snd_fd, &outsign, 2) != 2)
          WARNING(write);
      }
#if SDL_HAS_QUEUEAUDIO
      if(play_audio) {
        SDL_QueueAudio(psg_audio_device, &outsign, 2);
        //        printf("DEBUG: Audio device: %d\n", psg_audio_device);
      }
#endif
    }
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

  if(psgoutput
#if SDL_HAS_QUEUEAUDIO     
     || play_audio
#endif
     ) {
    psg_generate_presamples(tmpcpu/4);
    psg_generate_samples();
  }

  lastcpucnt = cpu->cycle;
}



