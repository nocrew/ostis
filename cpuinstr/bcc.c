#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"

#define BCC_NOT_TAKEN   1
#define BSR_PUSH_1      2
#define BSR_PUSH_2      3
#define BCC_PREFETCH_1  4
#define BCC_PREFETCH_2  5

// Only call with cc = 2-15.
static int condition_true(int cc)
{
  switch(cc) {
  case 2: /* BHI */
    return !CHKC && !CHKZ;
  case 3: /* BLS */
    return CHKC || CHKZ;
  case 4: /* BCC */
    return !CHKC;
  case 5: /* BCS */
    return CHKC;
  case 6: /* BNE */
    return !CHKZ;
  case 7: /* BEQ */
    return CHKZ;
  case 8: /* BVC */
    return !CHKV;
  case 9: /* BVS */
    return CHKV;
  case 10: /* BPL */
    return !CHKN;
  case 11: /* BMI */
    return CHKN;
  case 12: /* BGE */
    return (CHKN && CHKV) || (!CHKN && !CHKV);
  case 13: /* BLT */
    return (CHKN && !CHKV) || (!CHKN && CHKV);
  case 14: /* BGT */
    return (CHKN && CHKV && !CHKZ) || (!CHKN && !CHKV && !CHKZ);
  case 15: /* BLE */
    return CHKZ || (CHKN && !CHKV) || (!CHKN && CHKV);
  default:
    // It's an error to get here.
    return 0;
  }
}

/*
 * ---------------------------------------------------
 *                   |  Exec Time  |  Data Bus Usage
 *         Bcc       |    INSTR    |     INSTR
 * ------------------+-------------+-----------------
 * <label> :         |             |
 *  .B or .S :       |             |
 *   branch taken    | 10(2/0)     |   n   np np
 *   branch not taken|  8(1/0)     |  nn      np
 *  .W :             |             |
 *   branch taken    | 10(2/0)     |   n   np np
 *   branch not taken| 12(2/0)     |  nn   np np
 */
static void bcc(struct cpu *cpu, WORD op)
{
  static SLONG o;
  static int w;
  int cc ;

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

    cc = (op&0xf00)>>8;
    switch(cc) {
    case 0: /* BRA */
      cpu->pc += o;
      cpu->instr_state = BCC_PREFETCH_1;
      break;
    case 1: /* BSR */
      cpu->instr_state = BSR_PUSH_1;
      break;
    default:
      if(condition_true(cc)) {
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
    bus_write_word(cpu->a[7], cpu->pc >> 16);
    cpu->instr_state = BSR_PUSH_2;
    break;
  case BSR_PUSH_2:
    ADD_CYCLE(4);
    bus_write_word(cpu->a[7] + 2, cpu->pc);
    cpu->instr_state = BCC_PREFETCH_1;
    cpu->pc += o;
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
