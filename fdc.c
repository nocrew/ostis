#include "common.h"
#include "cpu.h"
#include "mfp.h"
#include "floppy.h"
#include "mmu.h"
#include "dma.h"
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

#define FDC_CMD_MULTSEC  ((FDC_CMD&0x10) == 0x10)

#define FDC_PENDING_TIME 512

#define CMD_TYPE_UNSET 0
#define CMD_TYPE_I     1
#define CMD_TYPE_II    2
#define CMD_TYPE_III   3
#define CMD_TYPE_IV    4

static int motor = MOTOR_OFF;
static int fdc_busy = FDC_IDLE;
static signed int pending_timer = 0;
static int step_direction = STEP_DIR_IN;
static int cmd_class = CMD_TYPE_UNSET;
static long lastcpucnt;
static int seek_error = 0;

static int fdc_reg[6];

HANDLE_DIAGNOSTICS(fdc);

static int fdc_read_sector();
static int fdc_write_sector();
static int fdc_read_address();
static int fdc_read_track();

BYTE fdc_status()
{
  int current_track,tr00;

  current_track = floppy_current_track();
  if(current_track == 0) {
    tr00 = 1;
  } else {
    tr00 = 0;
  }

  if(cmd_class != CMD_TYPE_I) {
    seek_error = 0;
    tr00 = 0;
  }
  
  return (motor<<7)|(seek_error<<4)|(tr00<<2)|fdc_busy;
}

void fdc_handle_control()
{
  TRACE("CONTROL: %02x", fdc_reg[FDC_REG_CONTROL]);

  mfp_set_GPIP(MFP_GPIP_FDC);
  motor = MOTOR_ON;
  fdc_busy = FDC_BUSY;
  seek_error = 0;
  
  if(FDC_CMD_RESTORE) {
    TRACE("CMD [RESTORE]");
    if(floppy_seek(0) == FLOPPY_ERROR) {
      seek_error = 1;
      TRACE("CMD [RESTORE] == SEEK_ERROR");
    } else {
      FDC_TRACK = 0;
    }
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_I;
  } else if(FDC_CMD_SEEK) {
    if(floppy_seek(fdc_reg[FDC_REG_DATA]) == FLOPPY_ERROR) {
      seek_error = 1;
      TRACE("CMD [SEEK] == SEEK_ERROR");
    } else {
      FDC_TRACK = floppy_current_track();
      TRACE("CMD [SEEK] == %d", FDC_TRACK);
    }
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_I;
  } else if(FDC_CMD_STEP) { 
    if(floppy_seek_rel(step_direction)) {
      seek_error = 1;
      TRACE("CMD [STEP] == SEEK_ERROR");
    } else {
      FDC_TRACK = floppy_current_track();
      TRACE("CMD [STEP] == %d", FDC_TRACK);
    }
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_I;
  } else if(FDC_CMD_STEPIN) {
    step_direction = STEP_DIR_IN;
    if(floppy_seek_rel(step_direction)) {
      seek_error = 1;
      TRACE("CMD [STEPIN] == SEEK_ERROR");
    } else {
      FDC_TRACK = floppy_current_track();
      TRACE("CMD [STEPIN] == %d", FDC_TRACK);
    }
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_I;
  } else if(FDC_CMD_STEPOUT) {
    step_direction = STEP_DIR_OUT;
    if(floppy_seek_rel(step_direction)) {
      seek_error = 1;
      TRACE("CMD [STEPOUT] == SEEK_ERROR");
    } else {
      FDC_TRACK = floppy_current_track();
      TRACE("CMD [STEPOUT] == %d", FDC_TRACK);
    }
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_I;
  } else if(FDC_CMD_READSEC) {
    if(fdc_read_sector() == FLOPPY_ERROR) {
      TRACE("CMD [READSEC] == READ_ERROR");
      dma_set_error();
    } else {
      TRACE("CMD [READSEC] == READ_OK");
      dma_clr_error();
      if(FDC_CMD_MULTSEC) {
        dma_inc_address(512 * dma_sector_count());
      } else {
        dma_inc_address(512);
      }
    }
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_II;
  } else if(FDC_CMD_WRITESEC) {
    TRACE("CMD [WRITESEC]");
    if(fdc_write_sector() == FLOPPY_ERROR) {
      dma_set_error();
    } else {
      dma_clr_error();
      if(FDC_CMD_MULTSEC) {
        dma_inc_address(512 * dma_sector_count());
      } else {
        dma_inc_address(512);
      }
    }
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_II;
  } else if(FDC_CMD_READADDR) {
    TRACE("CMD [READADDR]");
    if(fdc_read_address() == FLOPPY_ERROR) {
      dma_set_error();
    } else {
      dma_clr_error();
    }
    pending_timer = FDC_PENDING_TIME;
    cmd_class = CMD_TYPE_III;
  } else if(FDC_CMD_READTRK) {
    TRACE("CMD [READTRK]");
    if(fdc_read_track() == FLOPPY_ERROR) {
      dma_set_error();
    } else {
      dma_clr_error();
    }
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
  } else if(regnum == FDC_REG_SECTOR) {
    floppy_sector(data);
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

static int fdc_read_sector()
{
  return floppy_read_sector(dma_address(), dma_sector_count());
}

static int fdc_write_sector()
{
  return floppy_write_sector(dma_address(), dma_sector_count());
}

static int fdc_read_address()
{
  return floppy_read_address(dma_address());
}

static int fdc_read_track()
{
  return floppy_read_track(dma_address(), dma_sector_count());
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
