#include "common.h"
#include "cpu.h"
#include "mmu.h"

void dbcc(struct cpu *cpu, WORD op)
{
  SLONG o;
  int r;

  ENTER;
  
  r = op&7;
  o = mmu_read_word(cpu->pc);
  if(o&0x8000) o |= 0xffff0000;
  o -= 2;
  cpu->pc += 2;
  
  switch((op&0xf00)>>8) {
  case 0: /* DBT */
    ADD_CYCLE(12);
    return;
  case 1: /* DBF/DBRA */
    break;
  case 2: /* DBHI */
    if(!CHKC && !CHKZ) {
      ADD_CYCLE(12);
      return;
    }
    break;
  case 3: /* DBLS */
    if(CHKC || CHKZ) {
      ADD_CYCLE(12);
      return;
    }
    break;
  case 4: /* DBCC */
    if(!CHKC) {
      ADD_CYCLE(12);
      return;
    }
    break;
  case 5: /* DBCS */
    if(CHKC) {
      ADD_CYCLE(12);
      return;
    }
    break;
  case 6: /* DBNE */
    if(!CHKZ) {
      ADD_CYCLE(12);
      return;
    }
    break;
  case 7: /* DBEQ */
    if(CHKZ) {
      ADD_CYCLE(12);
      return;
    }
    break;
  case 8: /* DBVC */
    if(!CHKV) {
      ADD_CYCLE(12);
      return;
    }
    break;
  case 9: /* DBVS */
    if(CHKV) {
      ADD_CYCLE(12);
      return;
    }
    break;
  case 10: /* DBPL */
    if(!CHKN) {
      ADD_CYCLE(12);
      return;
    }
    break;
  case 11: /* DBMI */
    if(CHKN) {
      ADD_CYCLE(12);
      return;
    }
    break;
  case 12: /* DBGE */
    if((CHKN && CHKV) || (!CHKN && !CHKV)) {
      ADD_CYCLE(12);
      return;
    }
    break;
  case 13: /* DBLT */
    if((CHKN && !CHKV) || (!CHKN && CHKV)) {
      ADD_CYCLE(12);
      return;
    }
    break;
  case 14: /* DBGT */
    if((CHKN && CHKV && CHKZ) || (!CHKN && !CHKV && !CHKZ)) {
      ADD_CYCLE(12);
      return;
    }
    break;
  case 15: /* DBLE */
    if(CHKZ || (CHKN && !CHKV) || (!CHKN && CHKV)) {
      ADD_CYCLE(12);
      return;
    }
    break;
  }
  if(cpu->d[r]&0xffff) {
    cpu->d[r]=(cpu->d[r]&0xffff0000)|((cpu->d[r]-1)&0xffff);
    cpu->pc += o;
    ADD_CYCLE(12);
  } else {
    cpu->d[r] |= 0xffff;
    ADD_CYCLE(14);
  }
}

static struct cprint *dbcc_print(LONG addr, WORD op)
{
  LONG a;
  static char *cond[16] = {
    "DBT", "DBRA", "DBHI", "DBLS", "DBCC", "DBCS", "DBNE", "DBEQ",
    "DBVC", "DBVS", "DBPL", "DBMI", "DBGE", "DBLT", "DBGT", "DBLE"
  };
  struct cprint *ret;

  ret = cprint_alloc(addr);

  a = mmu_read_word_print(addr+ret->size);
  if(a&0x8000) a |= 0xffff0000;

  a += addr+ret->size;
  ret->size += 2;

  strcpy(ret->instr, cond[(op&0xf00)>>8]);
  cprint_set_label(a, NULL);
  if(cprint_find_label(a)) {
    sprintf(ret->data, "D%d,%s", op&0x7, cprint_find_label(a));
  } else {
    sprintf(ret->data, "D%d,$%x", op&0x7, a);
  }

  return ret;
}

void dbcc_init(void *instr[], void *print[])
{
  int i,r;
  
  for(r=0;r<8;r++) {
    for(i=0;i<0x16;i++) {
      instr[0x50c8|(i<<8)|r] = (void *)dbcc;
      print[0x50c8|(i<<8)|r] = (void *)dbcc_print;
    }
  }
}
