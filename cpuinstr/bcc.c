#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"

#define BCC_NOT_TAKEN   1
#define BSR_PUSH_1      2
#define BSR_PUSH_2      3
#define BCC_PREFETCH_1  4
#define BCC_PREFETCH_2  5

static void bcc(struct cpu *cpu, WORD op)
{
  SLONG o;
  static int w;
  static int saved_pc;

  ENTER;
  
  switch(cpu->instr_state) {
  case INSTR_STATE_NONE:
    ADD_CYCLE(2);

    w = 0;
    o = (SLONG)(SBYTE)(op&0xff);
    if(!o) {
      o = bus_read_word(cpu->pc);
      if(o&0x8000) o |= 0xffff0000;
      o -= 2;
      cpu->pc += 2;
      w = 1;
    }

    switch((op&0xf00)>>8) {
    case 0: /* BRA */
      cpu->pc += o;
      cpu->instr_state = BCC_PREFETCH_1;
      break;
    case 1: /* BSR */
      saved_pc = cpu->pc;
      cpu->pc += o;
      cpu->instr_state = BSR_PUSH_1;
      break;
    case 2: /* BHI */
      if(!CHKC && !CHKZ) {
        cpu->instr_state = BCC_PREFETCH_1;
        cpu->pc += o;
      } else {
        cpu->instr_state = BCC_NOT_TAKEN;
      }
      break;
    case 3: /* BLS */
      if(CHKC || CHKZ) {
        cpu->instr_state = BCC_PREFETCH_1;
        cpu->pc += o;
      } else {
        cpu->instr_state = BCC_NOT_TAKEN;
      }
      break;
    case 4: /* BCC */
      if(!CHKC) {
        cpu->instr_state = BCC_PREFETCH_1;
        cpu->pc += o;
      } else {
        cpu->instr_state = BCC_NOT_TAKEN;
      }
      break;
    case 5: /* BCS */
      if(CHKC) {
        cpu->instr_state = BCC_PREFETCH_1;
        cpu->pc += o;
      } else {
        cpu->instr_state = BCC_NOT_TAKEN;
      }
      break;
    case 6: /* BNE */
      if(!CHKZ) {
        cpu->instr_state = BCC_PREFETCH_1;
        cpu->pc += o;
      } else {
        cpu->instr_state = BCC_NOT_TAKEN;
      }
      break;
    case 7: /* BEQ */
      if(CHKZ) {
        cpu->instr_state = BCC_PREFETCH_1;
        cpu->pc += o;
      } else {
        cpu->instr_state = BCC_NOT_TAKEN;
      }
      break;
    case 8: /* BVC */
      if(!CHKV) {
        cpu->instr_state = BCC_PREFETCH_1;
        cpu->pc += o;
      } else {
        cpu->instr_state = BCC_NOT_TAKEN;
      }
      break;
    case 9: /* BVS */
      if(CHKV) {
        cpu->instr_state = BCC_PREFETCH_1;
        cpu->pc += o;
      } else {
        cpu->instr_state = BCC_NOT_TAKEN;
      }
      break;
    case 10: /* BPL */
      if(!CHKN) {
        cpu->instr_state = BCC_PREFETCH_1;
        cpu->pc += o;
      } else {
        cpu->instr_state = BCC_NOT_TAKEN;
      }
      break;
    case 11: /* BMI */
      if(CHKN) {
        cpu->instr_state = BCC_PREFETCH_1;
        cpu->pc += o;
      } else {
        cpu->instr_state = BCC_NOT_TAKEN;
      }
      break;
    case 12: /* BGE */
      if((CHKN && CHKV) || (!CHKN && !CHKV)) {
        cpu->instr_state = BCC_PREFETCH_1;
        cpu->pc += o;
      } else {
        cpu->instr_state = BCC_NOT_TAKEN;
      }
      break;
    case 13: /* BLT */
      if((CHKN && !CHKV) || (!CHKN && CHKV)) {
        cpu->instr_state = BCC_PREFETCH_1;
        cpu->pc += o;
      } else {
        cpu->instr_state = BCC_NOT_TAKEN;
      }
      break;
    case 14: /* BGT */
      if((CHKN && CHKV && !CHKZ) || (!CHKN && !CHKV && !CHKZ)) {
        cpu->instr_state = BCC_PREFETCH_1;
        cpu->pc += o;
      } else {
        cpu->instr_state = BCC_NOT_TAKEN;
      }
      break;
    case 15: /* BLE */
      if(CHKZ || (CHKN && !CHKV) || (!CHKN && CHKV)) {
        cpu->instr_state = BCC_PREFETCH_1;
        cpu->pc += o;
      } else {
        cpu->instr_state = BCC_NOT_TAKEN;
      }
      break;
    }
    break;
  case BCC_NOT_TAKEN:
    ADD_CYCLE(2);
    if(w)
      cpu->instr_state = BCC_PREFETCH_1;
    else
      cpu->instr_state = BCC_PREFETCH_2;
    break;
  case BSR_PUSH_1:
    ADD_CYCLE(4);
    cpu->a[7] -= 4;
    bus_write_word(cpu->a[7], saved_pc >> 16);
    cpu->instr_state = BSR_PUSH_2;
    break;
  case BSR_PUSH_2:
    ADD_CYCLE(4);
    bus_write_word(cpu->a[7] + 2, saved_pc);
    cpu->instr_state = BCC_PREFETCH_1;
    break;
  case BCC_PREFETCH_1:
    ADD_CYCLE(4);
    cpu->instr_state = BCC_PREFETCH_2;
    break;
  case BCC_PREFETCH_2:
    ADD_CYCLE(4);
    cpu->instr_state = INSTR_STATE_FINISHED;
    break;
  }
}

static struct cprint *bcc_print(LONG addr, WORD op)
{
  LONG a;
  int s;
  static char *cond[16] = {
    "BRA", "BSR", "BHI", "BLS", "BCC", "BCS", "BNE", "BEQ",
    "BVC", "BVS", "BPL", "BMI", "BGE", "BLT", "BGT", "BLE"
  };
  struct cprint *ret;

  ret = cprint_alloc(addr);

  a = op&0xff;
  if(a&0x80) a |= 0xffffff00;

  if(a) {
    s = 1;
  } else {
    s = 0;
    a = bus_read_word_print(addr+ret->size);
    if(a&0x8000) a |= 0xffff0000;
  }

  a += addr+ret->size;

  if(!s) ret->size += 2;

  strcpy(ret->instr, cond[(op&0xf00)>>8]);
  if(s) strcat(ret->instr, ".S");
  
  cprint_set_label(a, NULL);
  if(cprint_find_label(a)) {
    sprintf(ret->data, "%s", cprint_find_label(a));
  } else {
    sprintf(ret->data, "$%x", a);
  }

  return ret;
}

void bcc_instr_init(void *instr[], void *print[])
{
  int i;
  
  for(i=0;i<0x1000;i++) {
    instr[0x6000|i] = (void *)bcc;
    print[0x6000|i] = (void *)bcc_print;
  }
}
