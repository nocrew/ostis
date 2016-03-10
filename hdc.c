#include "common.h"
#include "cpu.h"
#include "mmu.h"
#include "dma.h"
#include "mfp.h"
#include "diag.h"

HANDLE_DIAGNOSTICS(hdc);

#define HDC_READ    0x08
#define HDC_WRITE   0x0a
#define HDC_INQUIRE 0x12

#define HDC_CTRL ((cmd&0xe0)>>5)
#define HDC_CMD (cmd&0x1f)
#define SECSIZE 512

BYTE cmd;
BYTE cmddata[5];
int cmddata_pos = 0;
int last_ctrl = 0;
int inquire_error = 0;
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
    mmu_write_byte(addr+i, buffer[i]);
  }
}

void hdc_write_sectors(LONG addr, LONG block, BYTE sectors)
{
  BYTE buffer[SECSIZE * 256];
  int i;
  
  if(!fp) return;

  TRACE("Write sectors: Addr: %06x  Block %06x  Sectors: %d", addr, block, sectors);
  for(i=0;i<SECSIZE*sectors;i++) {
    buffer[i] = mmu_read_byte(addr+i);
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
  if(last_ctrl != 0) return 0x02;
  if(inquire_error) return 0x20;
  
  return 0;
}

void hdc_add_cmddata(BYTE data)
{
  inquire_error = 0;
  if(last_ctrl == 0) {
    cmddata[cmddata_pos] = data;
  }
  cmddata_pos++;
  if(cmddata_pos >= 5) {
    last_ctrl = HDC_CTRL;
    if(last_ctrl == 0) {
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
        inquire_error = 1;
      } else {
        FATAL("Got HD command: %02x %02x %02x %02x %02x %02x",
              cmd, cmddata[0], cmddata[1], cmddata[2], cmddata[3], cmddata[4]);
      }
    }
    clr_cmddata();
  }
}

void hdc_set_cmd(BYTE data)
{
  cmd = data;
  DEBUG("HD CMD: Ctrl %d  Cmd %d", HDC_CTRL, HDC_CMD);
}

void hdc_init(char *hdfile)
{
  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(hdc, "HDC0");
  fp = fopen(hdfile, "r+");
  if(!fp) return;
}
