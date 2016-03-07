#include "common.h"
#include "cpu.h"
#include "mfp.h"
#include "floppy.h"
#include "mmu.h"
#include "state.h"
#include "diag.h"


#define FDC_REG_CONTROL 0
#define FDC_REG_STATUS  0
#define FDC_REG_TRACK   1
#define FDC_REG_SECTOR  2  
#define FDC_REG_DATA    3

#define MOTOR_ON  1
#define MOTOR_OFF 0

#define STEP_DIR_IN   1
#define STEP_DIR_OUT -1

#define FDC_IDLE 0
#define FDC_BUSY 1

#define FDC_CMD (fdc_reg[FDC_REG_CONTROL])
#define FDC_TRACK fdc_reg[FDC_REG_TRACK]

#define FDC_CMD_RESTORE  ((FDC_CMD&0xf0) == 0x00)
#define FDC_CMD_SEEK     ((FDC_CMD&0xf0) == 0x10)
#define FDC_CMD_STEP     ((FDC_CMD&0xe0) == 0x20)
#define FDC_CMD_STEPIN   ((FDC_CMD&0xe0) == 0x40)
#define FDC_CMD_STEPOUT  ((FDC_CMD&0xe0) == 0x60)
#define FDC_CMD_READSEC  ((FDC_CMD&0xe0) == 0x80)
#define FDC_CMD_WRITESEC ((FDC_CMD&0xe0) == 0xa0)
#define FDC_CMD_READADDR ((FDC_CMD&0xf0) == 0xc0)
#define FDC_CMD_READTRK  ((FDC_CMD&0xf0) == 0xe0)
#define FDC_CMD_WRITETRK ((FDC_CMD&0xf0) == 0xf0)
#define FDC_CMD_FORCEINT ((FDC_CMD&0xf0) == 0xd0)

#define FDC_PENDING_TIME 512

#define CMD_TYPE_UNSET 0
#define CMD_TYPE_I     1
#define CMD_TYPE_II    2
#define CMD_TYPE_III   3
#define CMD_TYPE_IV    4

static int motor = MOTOR_OFF;
static int fdc_busy = FDC_IDLE;
static int pending_timer = 0;
static int step_direction = STEP_DIR_IN;
static int cmd_class = CMD_TYPE_UNSET;
static long lastcpucnt;
static LONG dma_address;
static int dma_sector_count;

static int fdc_reg[6];

HANDLE_DIAGNOSTICS(fdc);

static void fdc_read_sector();
static void fdc_write_sector();
static void fdc_read_address();
static void fdc_read_track();

BYTE fdc_status()
{
  return (motor<<7)|fdc_busy;
}

void fdc_handle_control()
{
  TRACE("CONTROL: %d", fdc_reg[FDC_REG_CONTROL]);

  mfp_set_GPIP(MFP_GPIP_FDC);
  motor = MOTOR_ON;
  fdc_busy = FDC_BUSY;
  
  if(FDC_CMD_RESTORE) {
    TRACE("CMD [RESTORE]");
    FDC_TRACK = 0;
    floppy_seek(0);
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_I;
  } else if(FDC_CMD_SEEK) {
    FDC_TRACK = fdc_reg[FDC_REG_DATA];
    floppy_seek(FDC_TRACK);
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_I;
  } else if(FDC_CMD_STEP) {
    FDC_TRACK += step_direction;
    floppy_seek_rel(step_direction);
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_I;
  } else if(FDC_CMD_STEPIN) {
    step_direction = STEP_DIR_IN;
    FDC_TRACK += step_direction;
    floppy_seek_rel(step_direction);
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_I;
  } else if(FDC_CMD_STEPOUT) {
    step_direction = STEP_DIR_OUT;
    FDC_TRACK += step_direction;
    floppy_seek_rel(step_direction);
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_I;
  } else if(FDC_CMD_READSEC) {
    fdc_read_sector();
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_II;
  } else if(FDC_CMD_WRITESEC) {
    fdc_write_sector();
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_II;
  } else if(FDC_CMD_READADDR) {
    fdc_read_address();
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_III;
  } else if(FDC_CMD_READTRK) {
    fdc_read_track();
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_III;
  } else {
    FATAL("Unhandled CMD: %02x", FDC_CMD);
  }
}

void fdc_set_register(int regnum, BYTE data)
{
  if(fdc_busy) return;

  fdc_reg[regnum] = data;
  if(regnum == FDC_REG_CONTROL) {
    fdc_handle_control();
  }
}

BYTE fdc_get_register(int regnum)
{
  if(regnum == FDC_REG_STATUS) {
    return fdc_status();
  } else {
    return fdc_reg[regnum];
  }
}

void fdc_prepare_read(LONG addr, int count)
{
  dma_address = addr;
  dma_sector_count = count;
}

static void fdc_read_sector()
{
  floppy_read_sector(dma_address, dma_sector_count);
}

static void fdc_write_sector()
{
  floppy_write_sector(dma_address, dma_sector_count);
}

static void fdc_read_address()
{
  floppy_read_address(dma_address);
}

static void fdc_read_track()
{
  DEBUG("Read Track not implemented");
}

void fdc_do_interrupts(struct cpu *cpu)
{
  long tmpcpu;

  tmpcpu = cpu->cycle - lastcpucnt;

  if(tmpcpu < 0) {
    tmpcpu += MAX_CYCLE;
  }

  if(pending_timer > 0) {
    pending_timer -= tmpcpu;
  } else {
    if(fdc_busy) {
      TRACE("CMD DONE");
      pending_timer = 0;
      motor = MOTOR_OFF;
      mfp_clr_GPIP(MFP_GPIP_FDC);
      fdc_busy = FDC_IDLE;
    }
  }
  
  lastcpucnt = cpu->cycle;
}

void fdc_init()
{
  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(fdc, "FDC0");
}
