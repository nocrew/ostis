#ifndef CPU_H
#define CPU_H

struct cpu;

#include "common.h"
#include <SDL.h>

struct cpu {
  LONG a[8];
  LONG d[8];
  LONG usp;
  LONG ssp;
  LONG pc;
  WORD sr;
  uint64_t cycle;
  uint64_t start_cycle;
  unsigned int clock; // For now, just to display cycle in CLOCK().
  LONG icycle;
  int cyclecomp;
  int stopped;
  int debug_halted;
  int instr_state;
  WORD current_instr;
  WORD prefetched_instr;
  int has_prefetched;
  int ipl1, ipl2;

  
  /* instr_data_* is used to store prefetched information for various instructions 
   * 
   * instr_data_fetch:
   * Array of fetched words, either for instruction itself, or because of EA. 
   *
   * instr_data_word_ptr:
   * An array of pointers to words. This will be pointers to word-sections of the a[] and d[],
   * to simplify the read/write of words from/to memory. Maxsize of 32 is because MOVEM can have
   * all 16 registers, giving 32 possible words.
   *
   * instr_data_reg_num:
   * Array of register numbers corresponding to the word pointers in the previous array.
   * This is used in combination with the pointers by MOVEM to read from memory into registers
   *
   * instr_data_word_count:
   * A value saying how many words are in use in the array above, or for the fetch array
   *
   * instr_data_word_pos:
   * The current word in the array.
   *
   * instr_data_ea_reg:
   * The source/destination register (main register, not offset one)
   *
   * instr_data_ea_addr:
   * The source/destination offset to value in register
   *
   * instr_data_size:
   * 0 == word
   * 1 == long
   *
   * instr_data_step:
   * For .W writes, -2 or +2 to ea_addr between writes
   * For .L writes, -6 or +6 to ea_addr after every even write
   *   and +2 and -2 after every odd write
   *
   * instr_data_reg_change:
   * 0: Do not change register
   * 1: Add offset to register: (A0)+
   * 2: Add offset to register and add 2: -(A0)
   */

  WORD instr_data_fetch[4];
  WORD *instr_data_word_ptr[32];
  int instr_data_reg_num[32];
  int instr_data_word_count;
  int instr_data_word_pos;
  int instr_data_ea_reg;
  LONG instr_data_ea_addr;
  int instr_data_size;
  int instr_data_step;
};

struct cpu_state {
  char id[4];
  long size;
  char *data;
};

struct cpu_state *cpu_state_collect();
void cpu_state_restore(struct cpu_state *);

extern struct cpu *cpu;
extern int cprint_all;

#if 0
#define ADD_CYCLE(x) do { printf("DEBUG: CPU: [%s:%d] Add %d cycles\n", __FILE__, __LINE__, x); cpu->icycle += x; } while(0)
#else
#define ADD_CYCLE(x) do { cpu->icycle += x; } while(0)
#endif
#define MAX_CYCLE 8012800

#define MSKT 0x8000
#define MSKS 0x2000
#define MSKX 0x10
#define MSKN 0x8
#define MSKZ 0x4
#define MSKV 0x2
#define MSKC 0x1

#define SETX (cpu->sr |= MSKX)
#define SETN (cpu->sr |= MSKN)
#define SETZ (cpu->sr |= MSKZ)
#define SETV (cpu->sr |= MSKV)
#define SETC (cpu->sr |= MSKC)

#define CLRX (cpu->sr &= ~MSKX)
#define CLRN (cpu->sr &= ~MSKN)
#define CLRZ (cpu->sr &= ~MSKZ)
#define CLRV (cpu->sr &= ~MSKV)
#define CLRC (cpu->sr &= ~MSKC)

#define CHKS (cpu->sr & MSKS)
#define CHKT (cpu->sr & MSKT)
#define CHKX (cpu->sr & MSKX)
#define CHKN (cpu->sr & MSKN)
#define CHKZ (cpu->sr & MSKZ)
#define CHKV (cpu->sr & MSKV)
#define CHKC (cpu->sr & MSKC)

#define IPL ((cpu->sr & 0x700)>>8)
#define IPL_HBL 2
#define IPL_VBL 4
#define IPL_MFP 6
#define IPL_NO_AUTOVECTOR -1
#define IPL_EXCEPTION_VECTOR_OFFSET 24

#define VEC_BUSERR  2
#define VEC_ADDRERR 3
#define VEC_ILLEGAL 4
#define VEC_ZERODIV 5
#define VEC_CHK     6
#define VEC_TRAPV   7
#define VEC_PRIV    8
#define VEC_TRACE   9
#define VEC_LINEA  10
#define VEC_LINEF  11
#define VEC_SPUR   24

#define CPU_STACKFRAME_DATA  0x08

#define CPU_BUSERR_READ  0x10
#define CPU_BUSERR_WRITE 0
#define CPU_BUSERR_INSTR 0
#define CPU_BUSERR_DATA  0x08

#define CPU_ADDRERR_READ  0x10
#define CPU_ADDRERR_WRITE 0
#define CPU_ADDRERR_INSTR 0
#define CPU_ADDRERR_DATA  0x08

#define CPU_USE_CURRENT_PC 0
#define CPU_USE_LAST_PC    1

#define CPU_OK 0
#define CPU_BREAKPOINT 1
#define CPU_WATCHPOINT 2
#define CPU_TRACE_SINGLE 3

#define CPU_RUN 0
#define CPU_TRACE 1
#define CPU_DEBUG_RUN -1

#define CPU_DO_INTS 0
#define CPU_NO_INTS 1

#define CPU_WATCH_EQ 0
#define CPU_WATCH_NE 1
#define CPU_WATCH_GT 2
#define CPU_WATCH_GE 3
#define CPU_WATCH_LT 4
#define CPU_WATCH_LE 5

#define INSTR_STATE_NONE      -1
#define INSTR_STATE_FINISHED  -2

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define DREG_HWORD(c, dreg) (((void *)&c->d[dreg]))
#define DREG_LWORD(c, dreg) (((void *)&c->d[dreg])+2)
#define AREG_HWORD(c, areg) (((void *)&c->a[areg]))
#define AREG_LWORD(c, areg) (((void *)&c->a[areg])+2)
#else
#define DREG_HWORD(c, dreg) (((void *)&c->d[dreg])+2)
#define DREG_LWORD(c, dreg) (((void *)&c->d[dreg]))
#define AREG_HWORD(c, areg) (((void *)&c->a[areg])+2)
#define AREG_LWORD(c, areg) (((void *)&c->a[areg]))
#endif
#define REG_HWORD(c, reg) (reg > 7 ? AREG_HWORD(c, reg-8) : DREG_HWORD(c, reg))
#define REG_LWORD(c, reg) (reg > 7 ? AREG_LWORD(c, reg-8) : DREG_LWORD(c, reg))

#define EXTEND_BYTE(byte) (((byte)&0x80) ? ((byte)|0xff00) : (byte))
#define EXTEND_BY2L(byte) (((byte)&0x80) ? ((byte)|0xffffff00) : (byte))
#define EXTEND_WORD(word) (((word)&0x8000) ? ((word)|0xffff0000) : (word))

void cpu_init();
void cpu_init_clocked();
void cpu_halt_for_debug();
void cpu_enter_debugger();
WORD fetch_instr(struct cpu *);
void cpu_prefetch(void);
void cpu_clear_prefetch();
int cpu_step_instr(int);
void cpu_print_status(int);
void cpu_do_cycle(LONG);
void cpu_check_for_pending_interrupts();
void cpu_set_interrupt(int, int);
void cpu_set_exception(int);
void cpu_clr_exception(int);
int cpu_full_stacked_exception_pending();
void cpu_set_bus_error(int, LONG);
void cpu_set_address_error(int, LONG);
void cpu_add_debugpoint(LONG);
void cpu_set_sr(WORD);
int cpu_find_breakpoint_lowest_cnt(LONG);
void cpu_set_watchpoint(char *, int);
void cpu_set_breakpoint(LONG, int);
void cpu_print_breakpoints();
int cpu_unset_breakpoint(LONG);
int cpu_run(int);
int cpu_run_clocked(int);
int cpu_step_instr_clocked(int);
void cpu_do_clocked_cycle(LONG);
void cpu_reset_in(void);
void cpu_reset_out(void);
void cpu_add_extra_cycles(int);
void cpu_ipl1(void);
void cpu_ipl2(void);

void cprint_set_label(LONG, char *);
char *cprint_find_label(LONG);
struct cprint *cprint_instr(LONG);
void cprint_save_labels(char *);
void cprint_load_labels(char *);
int cprint_label_exists(char *);
LONG cprint_label_addr(char *);

static void cpu_set_flags_general(struct cpu *cpu, int mask,
				  int rm, int r)
{
  if(mask&MSKN) { if(rm) SETN; else CLRN; }
  if(mask&MSKX) { if(CHKC) SETX; else CLRX; }
  if(mask&MSKZ) { if(!r) SETZ; else CLRZ; }
}

/* SUB, SUBI, SUBQ */
static void cpu_set_flags_sub(struct cpu *cpu, int sm, int dm, int rm, int r)
{
  if((!sm && dm && !rm) || (sm && !dm && rm)) SETV; else CLRV;
  if((sm && !dm) || (rm && !dm) || (sm && rm)) SETC; else CLRC;
  cpu_set_flags_general(cpu, MSKN | MSKZ | MSKX, rm, r);
}

/* CAS, CAS2, CMP, CMPI, CMPM */
static void cpu_set_flags_cmp(struct cpu *cpu, int sm, int dm, int rm, int r)
{
  if((!sm && dm && !rm) || (sm && !dm && rm)) SETV; else CLRV;
  if((sm && !dm) || (rm && !dm) || (sm && rm)) SETC; else CLRC;
  cpu_set_flags_general(cpu, MSKN | MSKZ, rm, r);
}

/* ADD, ADDI, ADDQ */
static void cpu_set_flags_add(struct cpu *cpu, int sm, int dm, int rm, int r)
{
  if((sm && dm && !rm) || (!sm && !dm && rm)) SETV; else CLRV;
  if((sm && dm) || (!rm && dm) || (sm && !rm)) SETC; else CLRC;
  cpu_set_flags_general(cpu, MSKN | MSKZ | MSKX, rm, r);
}

/* MOVE, MOVEQ, AND, ANDI, MULU, OR, ORI, TST */
static void cpu_set_flags_move(struct cpu *cpu, int rm, int r)
{
  if(rm) SETN; else CLRN;
  if(!r) SETZ; else CLRZ;
  CLRV;
  CLRC;
}

/* NEG */
static void cpu_set_flags_neg(struct cpu *cpu, int dm, int rm, int r)
{
  if(dm && rm) SETV; else CLRV;
  if(dm || rm) SETC; else CLRC;
  cpu_set_flags_general(cpu, MSKX | MSKN | MSKZ, rm, r);
}

/* DIVU */
static void cpu_set_flags_divu(struct cpu *cpu, int rm, int r, int v)
{
  if(rm) SETN; else CLRN;
  if(!r) SETZ; else CLRZ;
  if(v) SETV; else CLRV;
  CLRC;
}

/* DIVS */
static void cpu_set_flags_divs(struct cpu *cpu, int rm, int r)
{
  if(rm) SETN; else CLRN;
  if(!r) SETZ; else CLRZ;
  if(((r&0xffff8000) != 0) && ((r&0xffff8000) != 0xffff8000)) SETV; else CLRV;
  CLRC;
}

/* CLR */
static void cpu_set_flags_clr(struct cpu *cpu)
{
  CLRN;
  CLRV;
  CLRC;
  SETZ;
}

/* LSR */
static void cpu_set_flags_lsr(struct cpu *cpu, int rm, int r, int cnt, int hb)
{
  CLRV;
  if(cnt) {
    if(hb) SETC; else CLRC;
    cpu_set_flags_general(cpu, MSKX | MSKN | MSKZ, rm, r);
  } else {
    CLRC;
    cpu_set_flags_general(cpu, MSKN | MSKZ, rm, r);
  }
}

/* ROXL, ROXR */
static void cpu_set_flags_roxl(struct cpu *cpu, int rm, int r, int cnt, int hb)
{
  CLRV;
  if(cnt) {
    if(hb) SETC; else CLRC;
    cpu_set_flags_general(cpu, MSKX | MSKN | MSKZ, rm, r);
  } else {
    if(CHKX) SETX; else CLRX;
    cpu_set_flags_general(cpu, MSKN | MSKZ, rm, r);
  }
}

/* ROL */
static void cpu_set_flags_rol(struct cpu *cpu, int rm, int r, int cnt, int hb)
{
  CLRV;
  if(cnt) {
    if(hb) SETC; else CLRC;
    cpu_set_flags_general(cpu, MSKN | MSKZ, rm, r);
  } else {
    CLRC;
    cpu_set_flags_general(cpu, MSKN | MSKZ, rm, r);
  }
}

/* ASL */
static void cpu_set_flags_asl(struct cpu *cpu, int rm, int r, int cnt,
			      int hb, int vb)
{
  if(cnt) {
    if(hb) SETC; else CLRC;
    if(vb) SETV; else CLRV;
    cpu_set_flags_general(cpu, MSKX | MSKN | MSKZ, rm, r);
  } else {
    CLRV;
    CLRC;
    cpu_set_flags_general(cpu, MSKN | MSKZ, rm, r);
  }
}

/* ADDX */
static void cpu_set_flags_addx(struct cpu *cpu, int sm, int dm, int rm, int r)
{
  if(r) CLRZ;
  if((sm && dm && !rm) || (!sm && !dm && rm)) SETV; else CLRV;
  if((sm && dm) || (!rm && dm) || (sm && !rm)) SETC; else CLRC;
  cpu_set_flags_general(cpu, MSKN | MSKX, rm, r);
}

/* SUBX */
static void cpu_set_flags_subx(struct cpu *cpu, int sm, int dm, int rm, int r)
{
  if(r) CLRZ;
  if((!sm && dm && !rm) || (sm && !dm && rm)) SETV; else CLRV;
  if((sm && !dm) || (rm && !dm) || (sm && rm)) SETC; else CLRC;
  cpu_set_flags_general(cpu, MSKN | MSKX, rm, r);
}

/* NEGX */
static void cpu_set_flags_negx(struct cpu *cpu, int dm, int rm, int r)
{
  if(dm && rm) SETV; else CLRV;
  if(dm || rm) SETC; else CLRC;
  if(r) CLRZ;
  cpu_set_flags_general(cpu, MSKN | MSKX, rm, r);
}

#endif
