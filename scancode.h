#ifndef SCANCODE_H
#define SCANCODE_H

static int scancode[256] = {
  0,0,0,0,0,0,0,0,
  14,15,0,0,0,28,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,1,0,0,0,0,

  57,0,0,0,0,0,0,40, /* SPACE */
  0,0,0,0,51,12,52,53,
  11,2,3,4,5,6,7,8,
  9,10,0,39,0,13,0,0,

  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,26,96,27,0,0,

  41,30,48,46,32,18,33,34,
  35,23,36,37,38,50,49,24,
  25,16,19,31,20,22,47,17,
  45,21,44,0,0,0,0,0,

  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,

  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,

  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,

  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0
};

#define SCAN_F1 0x3b
#define SCAN_LSHIFT 0x2A
#define SCAN_RSHIFT 0x36
#define SCAN_CONTROL 0x1D
#define SCAN_ALT 0x38
#define SCAN_UP 0x48
#define SCAN_DOWN 0x50
#define SCAN_LEFT 0x4b
#define SCAN_RIGHT 0x4d
#define SCAN_INSERT 0x52
#define SCAN_DELETE 0x53

#define SCAN_LEFT_BUTTON 0x02
#define SCAN_RIGHT_BUTTON 0x01

#endif
