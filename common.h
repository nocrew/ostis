#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "screen.h"

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t LONG;

typedef int8_t SBYTE;
typedef int16_t SWORD;
typedef int32_t SLONG;

#define INCLUDE_RTC 0

#define SIZE_B 0
#define SIZE_W 1
#define SIZE_L 2
#define MASK_B 0xff
#define MASK_W 0xffff
#define MASK_L 0xffffffff

#include "shifter.h"

#define ENTER 
#define EXIT 
#define ENDLOOP

#define SCREEN_NORMAL 0

#define MAX(x, y) (((x)>(y))?(x):(y))
#define MIN(x, y) (((x)<(y))?(x):(y))

#define WARNING(F) \
  fprintf(stderr, "WARNING: unexpected return value from %s at %s:%d\n", #F, __FILE__, __LINE__);

extern int debugger;
extern int clocked_cpu;
extern int ppmoutput;
extern int psgoutput;
extern int play_audio;
extern int audio_device;
extern int monitor_sm124;
extern int crop_screen;
#if TEST_BUILD
extern char *testcase_name;
#endif

extern void *xmalloc(size_t);
extern char *xstrdup(char *);

#endif

