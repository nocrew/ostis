#include "common.h"
#include "cpu.h"
#include "mmu.h"
#include "dma.h"
#include "mfp.h"
#include "diag.h"

HANDLE_DIAGNOSTICS(hdc);

#define HDC_TEST_READY 0x00
#define HDC_READ       0x08
#define HDC_WRITE      0x0a
#define HDC_INQUIRE    0x12

#define HDC_RETURN_OK    0x00
#define HDC_RETURN_ERROR 0x02
#define HDC_RETURN_NOCMD 0x20

#define HDC_CTRL ((cmd&0xe0)>>5)
#define HDC_CMD (cmd&0x1f)
#define SECSIZE 512

BYTE cmd;
BYTE cmddata[5];
int cmddata_pos = 0;
int last_ctrl = 0;
int return_code = 0;
static int connected = 0;
FILE *fp;

void hdc_read_sectors(LONG addr, LONG block, BYTE sectors)
{
  BYTE buffer[SECSIZE * 256];
  int i;
  
  if(!fp) return;
  fseek(fp, block * SECSIZE, SEEK_SET);
  if(fread(buffer, 1, SECSIZE * sectors, fp) != (SECSIZE * sectors)) {
    fclose(fp);
    fp = NULL;
    ERROR("Unable to read file: Addr: %d  Block %d  Sectors: %d", addr, block, sectors);
    return;
  }
  TRACE("Read sectors: Addr: %06x  Block %06x  Sectors: %d", addr, block, sectors);
  for(i=0;i<SECSIZE*sectors;i++) {
    bus_write_byte(addr+i, buffer[i]);
  }
}

void hdc_write_sectors(LONG addr, LONG block, BYTE sectors)
{
  BYTE buffer[SECSIZE * 256];
  int i;
  
  if(!fp) return;

  TRACE("Write sectors: Addr: %06x  Block %06x  Sectors: %d", addr, block, sectors);
  for(i=0;i<SECSIZE*sectors;i++) {
    buffer[i] = bus_read_byte(addr+i);
  }

  fseek(fp, block * SECSIZE, SEEK_SET);
  if(fwrite(buffer, 1, SECSIZE * sectors, fp) != (SECSIZE * sectors)) {
    fclose(fp);
    fp = NULL;
    ERROR("Unable to write file: Addr: %d  Block %d  Sectors: %d", addr, block, sectors);
    return;
  }
}

static void clr_cmddata()
{
  cmddata[0] = 0;
  cmddata[1] = 0;
  cmddata[2] = 0;
  cmddata[3] = 0;
  cmddata[4] = 0;
  cmddata_pos = 0;
}

LONG hdc_cmd_block()
{
  return (cmddata[0]<<16)|(cmddata[1]<<8)|cmddata[2];
}

BYTE hdc_get_status()
{
  if(last_ctrl != 0) return HDC_RETURN_ERROR;
  if(!connected) return HDC_RETURN_ERROR;
  
  return return_code;
}

void hdc_add_cmddata(BYTE data)
{
  if(last_ctrl == 0) {
    cmddata[cmddata_pos] = data;
  }
  cmddata_pos++;
  if(cmddata_pos >= 5) {
    last_ctrl = HDC_CTRL;
    if(last_ctrl == 0 && connected) {
      DEBUG("Got HD command: %02x %02x %02x %02x %02x %02x",
            cmd, cmddata[0], cmddata[1], cmddata[2], cmddata[3], cmddata[4]);
      if(HDC_CMD == HDC_READ) {
        mfp_set_GPIP(MFP_GPIP_HDC);
        hdc_read_sectors(dma_address(), hdc_cmd_block(), cmddata[3]);
        mfp_clr_GPIP(MFP_GPIP_HDC);
      } else if(HDC_CMD == HDC_WRITE) {
        mfp_set_GPIP(MFP_GPIP_HDC);
        hdc_write_sectors(dma_address(), hdc_cmd_block(), cmddata[3]);
        mfp_clr_GPIP(MFP_GPIP_HDC);
      } else if(HDC_CMD == HDC_INQUIRE) {
        return_code = HDC_RETURN_NOCMD;
      } else if(HDC_CMD == HDC_TEST_READY) {
        return_code = HDC_RETURN_OK;
      } else {
        return_code = HDC_RETURN_ERROR;
        ERROR("Got unknown HD command: %02x %02x %02x %02x %02x %02x",
              cmd, cmddata[0], cmddata[1], cmddata[2], cmddata[3], cmddata[4]);
      }
    }
    clr_cmddata();
  }
}

void hdc_set_cmd(BYTE data)
{
  return_code = HDC_RETURN_OK;
  cmd = data;
  DEBUG("HD CMD: Ctrl %d  Cmd %d", HDC_CTRL, HDC_CMD);
}

void hdc_init(char *hdfile)
{
  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(hdc, "HDC0");
  fp = NULL;
  if(hdfile) {
    fp = fopen(hdfile, "r+");
    if(fp) {
      connected = 1;
    }
  }
}
