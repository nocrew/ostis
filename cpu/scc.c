#include "common.h"
#include "cpu.h"
#include "ea.h"

void scc(struct cpu *cpu, WORD op)
{
  int r;

  ENTER;
  
  if(op&0x38) {
    r = 1;
    ADD_CYCLE(4);
  } else {
    r = 0;
    ADD_CYCLE(6);
  }
  
  switch((op&0xf00)>>8) {
  case 0: /* ST */
    ADD_CYCLE(2);
    ea_write_byte(cpu, op&0x3f, 0xff);
    return;
  case 1: /* SF */
    break;
  case 2: /* SHI */
    if(!CHKC && !CHKZ) {
      ADD_CYCLE(2);
      ea_write_byte(cpu, op&0x3f, 0xff);
      return;
    }
    break;
  case 3: /* SLS */
    if(CHKC || CHKZ) {
      ADD_CYCLE(2);
      ea_write_byte(cpu, op&0x3f, 0xff);
      return;
    }
    break;
  case 4: /* SCC */
    if(!CHKC) {
      ADD_CYCLE(2);
      ea_write_byte(cpu, op&0x3f, 0xff);
      return;
    }
    break;
  case 5: /* SCS */
    if(CHKC) {
      ADD_CYCLE(2);
      ea_write_byte(cpu, op&0x3f, 0xff);
      return;
    }
    break;
  case 6: /* SNE */
    if(!CHKZ) {
      ADD_CYCLE(2);
      ea_write_byte(cpu, op&0x3f, 0xff);
      return;
    }
    break;
  case 7: /* SEQ */
    if(CHKZ) {
      ADD_CYCLE(2);
      ea_write_byte(cpu, op&0x3f, 0xff);
      return;
    }
    break;
  case 8: /* SVC */
    if(!CHKV) {
      ADD_CYCLE(2);
      ea_write_byte(cpu, op&0x3f, 0xff);
      return;
    }
    break;
  case 9: /* SVS */
    if(CHKV) {
      ADD_CYCLE(2);
      ea_write_byte(cpu, op&0x3f, 0xff);
      return;
    }
    break;
  case 10: /* SPL */
    if(!CHKN) {
      ADD_CYCLE(2);
      ea_write_byte(cpu, op&0x3f, 0xff);
      return;
    }
    break;
  case 11: /* SMI */
    if(CHKN) {
      ADD_CYCLE(2);
      ea_write_byte(cpu, op&0x3f, 0xff);
      return;
    }
    break;
  case 12: /* SGE */
    if((CHKN && CHKV) || (!CHKN && !CHKV)) {
      ADD_CYCLE(2);
      ea_write_byte(cpu, op&0x3f, 0xff);
      return;
    }
    break;
  case 13: /* SLT */
    if((CHKN & !CHKV) | (!CHKN & CHKV)) {
      ADD_CYCLE(2);
      ea_write_byte(cpu, op&0x3f, 0xff);
      return;
    }
    break;
  case 14: /* SGT */
    if((CHKN && CHKV && CHKZ) || (!CHKN && !CHKV && !CHKZ)) {
      ADD_CYCLE(2);
      ea_write_byte(cpu, op&0x3f, 0xff);
      return;
    }
    break;
  case 15: /* SLE */
    if(CHKZ || (CHKN && !CHKV) || (!CHKN && CHKV)) {
      ADD_CYCLE(2);
      ea_write_byte(cpu, op&0x3f, 0xff);
      return;
    }
    break;
  }
  if(!r) {
    ADD_CYCLE(2);
  }
  ea_write_byte(cpu, op&0x3f, 0x00);
}

static struct cprint *scc_print(LONG addr, WORD op)
{
  static char *cond[16] = {
    "ST", "SF", "SHI", "SLS", "SCC", "SCS", "SNE", "SEQ",
    "SVC", "SVS", "SPL", "SMI", "SGE", "SLT", "SGT", "SLE"
  };
  struct cprint *ret;

  ret = cprint_alloc(addr);

  strcpy(ret->instr, cond[(op&0xf00)>>8]);
  ea_print(ret, op&0x3f, 0);

  return ret;
}

void scc_init(void *instr[], void *print[])
{
  int i,r;
  
  for(r=0;r<16;r++) {
    for(i=0;i<0x40;i++) {
      instr[0x50c0|(r<<8)|i] = (void *)scc;
      print[0x50c0|(r<<8)|i] = (void *)scc_print;
    }
  }
}
