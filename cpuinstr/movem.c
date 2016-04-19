#include "common.h"
#include "cpu.h"
#include "cprint.h"
#include "mmu.h"
#include "ea.h"

#define MOVEM_REGTOMEM(mode) ((mode&0x400) == 0x000)
#define MOVEM_MEMTOREG(mode) ((mode&0x400) == 0x400)

#define MOVEM_SIZE_W(mode) ((mode&0x40) == 0x00)
#define MOVEM_SIZE_L(mode) ((mode&0x40) == 0x40)

#define MOVEM_EA_FETCH   1
#define MOVEM_PARSE      2
#define MOVEM_WRITE      3
#define MOVEM_READ       4
#define MOVEM_EXTRA_READ 5

#define EA_REG_IN_RMASK(rnum, rmask) (rmask&(1<<(rnum+8)))

static WORD fetch_word(struct cpu *cpu)
{
  WORD data;

  data = bus_read_word(cpu->pc);
  cpu->pc += 2;
  return data;
}

static void set_state(struct cpu *cpu, int state)
{
  cpu->instr_state = state;
}

static void parse(struct cpu *cpu, WORD op)
{
  int i;
  int word_count = 0;
  int step = 2;
  int reverse = 0;
  WORD rmask = cpu->instr_data_fetch[0];
  void *tmp;
  int rnum;

  cpu->instr_data_size = MOVEM_SIZE_L(op) ? 1 : 0;
  cpu->instr_data_step = step;
  cpu->instr_data_ea_reg = EA_MODE_REG(op);
  cpu->instr_data_word_pos = 0;
  cpu->instr_data_ea_addr = ea_addr(cpu, cpu->instr_data_size, op, &cpu->instr_data_fetch[1]);

  /* Compensate for having moved PC when reading RMASK */
  if(EA_MODE_POFF(op) || EA_MODE_PROF(op)) {
    cpu->instr_data_ea_addr -= 2;
  }
  
  if(EA_MODE_ADEC(op)) {
    reverse = 1;
    //    cpu->instr_data_ea_addr += 2;
    cpu->instr_data_step = -cpu->instr_data_step;
  }
  
  for(i=0;i<16;i++) {
    if(reverse) {
      if(rmask & (1<<(15-i))) {
        if(cpu->instr_data_size) {
          cpu->instr_data_reg_num[word_count] = i;
          cpu->instr_data_word_ptr[word_count++] = REG_HWORD(cpu, i);
        }
        cpu->instr_data_reg_num[word_count] = i;
        cpu->instr_data_word_ptr[word_count++] = REG_LWORD(cpu, i);
      }
    } else {
      if(rmask & (1<<i)) {
        if(cpu->instr_data_size) {
          cpu->instr_data_reg_num[word_count] = i;
          cpu->instr_data_word_ptr[word_count++] = REG_HWORD(cpu, i);
        }
        cpu->instr_data_reg_num[word_count] = i;
        cpu->instr_data_word_ptr[word_count++] = REG_LWORD(cpu, i);
      }
    }
  }

  /* Reverse order of word ptr array */
  if(reverse) {
    for(i=0;i<word_count/2;i++) {
      tmp = cpu->instr_data_word_ptr[i];
      cpu->instr_data_word_ptr[i] = cpu->instr_data_word_ptr[word_count-i-1];
      cpu->instr_data_word_ptr[word_count-i-1] = tmp;
      rnum = cpu->instr_data_reg_num[i];
      cpu->instr_data_reg_num[i] = cpu->instr_data_reg_num[word_count-i-1];
      cpu->instr_data_reg_num[word_count-i-1] = rnum;
    }
  }
  
  cpu->instr_data_word_count = word_count;
}

static void write_word(struct cpu *cpu)
{
  LONG addr;
  WORD data;

  /* Predec */
  if(cpu->instr_data_step < 0) {
    cpu->instr_data_ea_addr += cpu->instr_data_step;
  }
  
  addr = cpu->instr_data_ea_addr;
  data = *((WORD *)cpu->instr_data_word_ptr[cpu->instr_data_word_pos]);

  bus_write_word(addr, data);

  /* Postinc */
  if(cpu->instr_data_step > 0) {
    cpu->instr_data_ea_addr += cpu->instr_data_step;
  }

  cpu->instr_data_word_pos++;
}

static void read_word(struct cpu *cpu)
{
  LONG addr;
  WORD data;
  int rnum;

  addr = cpu->instr_data_ea_addr;
  data = bus_read_word(addr);
  rnum = cpu->instr_data_reg_num[cpu->instr_data_word_pos];

  *((WORD *)cpu->instr_data_word_ptr[cpu->instr_data_word_pos]) = data;
  if(!cpu->instr_data_size) {
    if(rnum > 7) {
      cpu->a[rnum-8] = EXTEND_WORD(cpu->a[rnum-8]&0xffff);
    } else { 
      cpu->d[rnum] = EXTEND_WORD(cpu->d[rnum]&0xffff);
    }
  }

  cpu->instr_data_ea_addr += cpu->instr_data_step;
  cpu->instr_data_word_pos++;
}

static void adjust_register(struct cpu *cpu, WORD op)
{
  /* Add or subtract EA register when post-inc or pre-dec, except when
   * EA register was fetched from memory during the instruction
   */
  if(EA_MODE_AINC(op) && !EA_REG_IN_RMASK(cpu->instr_data_ea_reg, cpu->instr_data_fetch[0])) {
    cpu->a[cpu->instr_data_ea_reg] += cpu->instr_data_word_count * 2;
  } else if(EA_MODE_ADEC(op)) {
    cpu->a[cpu->instr_data_ea_reg] -= cpu->instr_data_word_count * 2;
  }
}

/*
 * -------------------------------------------------------------------------
 *                   |    Exec Time    |               Data Bus Usage
 *       MOVEM       |      INSTR      |                  INSTR
 * ------------------+-----------------+------------------------------------
 * M --> R           |                 |
 *   .W              |                 |
 *     (An)          | 12+4m(3+m/0)    |                np (nr)*    nr np
 *     (An)+         | 12+4m(3+m/0)    |                np (nr)*    nr np
 *     (d16,An)      | 16+4m(4+m/0)    |             np np (nr)*    nr np
 *     (d8,An,Xn)    | 18+4m(4+m/0)    |          np n  np (nr)*    nr np
 *     (xxx).W       | 16+4m(4+m/0)    |             np np (nr)*    nr np
 *     (xxx).L       | 20+4m(5+m/0)    |          np np np (nr)*    nr np
 *   .L              |                 |
 *     (An)          | 12+8m(3+2m/0)   |                np nR (nr nR)* np
 *     (An)+         | 12+8m(3+2m/0)   |                np nR (nr nR)* np
 *     (d16,An)      | 16+8m(4+2m/0)   |             np np nR (nr nR)* np
 *     (d8,An,Xn)    | 18+8m(4+2m/0)   |          np n  np nR (nr nR)* np
 *     (xxx).W       | 16+8m(4+2m/0)   |             np np nR (nr nR)* np
 *     (xxx).L       | 20+8m(5+2m/0)   |          np np np nR (nr nR)* np
 * R --> M           |                 |
 *   .W              |                 |
 *     (An)          |  8+4m(2/m)      |                np (nw)*       np
 *     -(An)         |  8+4m(2/m)      |                np (nw)*       np
 *     (d16,An)      | 12+4m(3/m)      |             np np (nw)*       np
 *     (d8,An,Xn)    | 14+4m(3/m)      |          np n  np (nw)*       np
 *     (xxx).W       | 12+4m(3/m)      |             np np (nw)*       np
 *     (xxx).L       | 16+4m(4/m)      |          np np np (nw)*       np
 *   .L              |                 |
 *     (An)          |  8+8m(2/2m)     |                np (nW nw)*    np
 *     -(An)         |  8+8m(2/2m)     |                np (nw nW)*    np
 *     (d16,An)      | 12+8m(3/2m)     |             np np (nW nw)*    np
 *     (d8,An,Xn)    | 14+8m(3/2m)     |          np n  np (nW nw)*    np
 *     (xxx).W       | 12+8m(3/2m)     |             np np (nW nw)*    np
 *     (xxx).L       | 16+8m(4/2m)     |          np np np (nW nw)*    np
 * NOTES :
 *   .'m' is the number of registers to move.
 *   .'(nr)*' should be replaced by m consecutive 'nr'
 *   .'(nw)*' should be replaced by m consecutive 'nw'
 *   .'(nR nr)*' should be replaced by m consecutive 'nR nr'
 *   .'(nW nw)*' (or '(nw nW)*') should be replaced by m consecutive 'nW nw'
 *    (or m consecutive 'nw nW').
 *   .In M --> R mode, an extra bus read cycle occurs. This extra access can
 *    cause errors (for exemple if occuring beyond the bounds of physical
 *    memory).
 *   .In M --> R mode, MOVEM.W sign-extends words moved to data registers.
 */
static void movem(struct cpu *cpu, WORD op)
{
  switch(cpu->instr_state) {


    /* Initial state 
     * Read rmask word
     * Calculate number of words of EA to fetch
     * Wait 4 cycles
     */
  case INSTR_STATE_NONE: 
    cpu->instr_data_fetch[0] = fetch_word(cpu);
    cpu->instr_data_word_count = EA_FETCH_COUNT(op);
    cpu->instr_data_word_pos = 0;

    /* If there are EA words to fetch, go to EA fetch state, 
     * otherwise everything is fetched, and can be parsed
     */
    if(cpu->instr_data_word_count > 0) {
      set_state(cpu, MOVEM_EA_FETCH);
    } else {
      set_state(cpu, MOVEM_PARSE);
    }
    ADD_CYCLE(4);
    break;

    
    /* Fetch one word for EA, remain in state if there
     * are more words to fetch
     */
  case MOVEM_EA_FETCH:
    cpu->instr_data_fetch[cpu->instr_data_word_pos + 1] = fetch_word(cpu);
    cpu->instr_data_word_pos++;
    /* Still EA to fetch, keep current state, otherwise move on to parse */
    if(cpu->instr_data_word_pos >= cpu->instr_data_word_count) {
      set_state(cpu, MOVEM_PARSE);
    }
    ADD_CYCLE(4);
    break;

    
    /* Parse instruction
     * Wait 4 cycles
     */
  case MOVEM_PARSE:
    parse(cpu, op);
    if(MOVEM_MEMTOREG(op)) {
      set_state(cpu, MOVEM_READ);
    } else {
      set_state(cpu, MOVEM_WRITE);
    }
    ADD_CYCLE(4);
    break;

    
    /* Read/write word
     * Wait 4 cycles
     * If wordcount > 0, remain in the same state,
     * otherwise go to finished state
     */
  case MOVEM_WRITE:
    write_word(cpu);
    if(cpu->instr_data_word_pos >= cpu->instr_data_word_count) {
      adjust_register(cpu, op);
      set_state(cpu, INSTR_STATE_FINISHED);
    }
    ADD_CYCLE(4);
    break;
  case MOVEM_READ:
    read_word(cpu);
    if(cpu->instr_data_word_pos >= cpu->instr_data_word_count) {
      set_state(cpu, MOVEM_EXTRA_READ);
    }
    ADD_CYCLE(4);
    break;

    
    /* During a Mem to Reg operation, MOVEM does an extra read
     * at the end of the sequence, which must be done, because
     * it can actually cause a BUS ERROR even though it will never
     * populate any register
     */
  case MOVEM_EXTRA_READ:
    bus_read_word(cpu->instr_data_ea_addr);
    adjust_register(cpu, op);
    set_state(cpu, INSTR_STATE_FINISHED);
    ADD_CYCLE(4);
  }
}

static void rmask_print(struct cprint *cprint, int rmask)
{
  int d,a;
  int first,end,last,sep;

  first = 1;
  sep = 0;
  end = 0;
  last = 0;

  for(d=0;d<8;d++) {
    if(rmask&(1<<d)) {
      if(!last) {
        if(sep) strcat(cprint->data, "/");
        sprintf(cprint->data, "%sD%d", cprint->data, d);
        first = 0;
        end = 0;
      } else {
        end = d;
      }
      last = 1;
    } else {
      if(last) {
        if(end) sprintf(cprint->data, "%s-D%d", cprint->data, end);
      }
      if(!first) sep = 1;
      last = 0;
    }
  }
  if(end == 7) strcat(cprint->data, "-D7");

  last = 0;
  end = 0;
  if(!first) sep = 1;

  for(a=0;a<8;a++) {
    if(rmask&(1<<(a+8))) {
      if(!last) {
        if(sep) strcat(cprint->data, "/");
        sprintf(cprint->data, "%sA%d", cprint->data, a);
        first = 0;
        end = 0;
      } else {
        end = a;
      }
      last = 1;
    } else {
      if(last) {
        if(end) sprintf(cprint->data, "%s-A%d", cprint->data, end);
      }
      if(!first) sep = 1;
      last = 0;
    }
  }
  if(end == 7) strcat(cprint->data, "-A7");
}

static int rmask_reverse(int rmask)
{
  int i,t;

  t = 0;

  for(i=0;i<16;i++) {
    t<<=1;
    t |= rmask&1;
    rmask>>=1;
  }
  return t;
}

static struct cprint *movem_print(LONG addr, WORD op)
{
  int rmask;
  struct cprint *ret;
  
  ret = cprint_alloc(addr);

  rmask = bus_read_word_print(addr+ret->size);
  ret->size += 2;
  
  if(((op&0x38)>>3) == 4) {
    rmask = rmask_reverse(rmask);
  }

  switch(op&0x40) {
  case 0:
    strcpy(ret->instr, "MOVEM.W");
    break;
  case 0x40:
    strcpy(ret->instr, "MOVEM.L");
    break;
  }

  if(op&0x400) {
    ea_print(ret, op&0x3f, 1);
    strcat(ret->data, ",");
    rmask_print(ret, rmask);
  } else {
    rmask_print(ret, rmask);
    strcat(ret->data, ",");
    ea_print(ret, op&0x3f, 1);
  }

  return ret;
}

void movem_instr_init(void *instr[], void *print[])
{
  int i;
  
  for(i=0;i<0x40;i++) {
    if(ea_valid(i, EA_INVALID_MEM|EA_INVALID_INC)) {
      instr[0x4880|i] = movem;
      instr[0x48c0|i] = movem;
      print[0x4880|i] = movem_print;
      print[0x48c0|i] = movem_print;
    }
    if(ea_valid(i, EA_INVALID_I|EA_INVALID_A|EA_INVALID_D|EA_INVALID_DEC)) {
      instr[0x4c80|i] = movem;
      instr[0x4cc0|i] = movem;
      print[0x4c80|i] = movem_print;
      print[0x4cc0|i] = movem_print;
    }
  }
}
