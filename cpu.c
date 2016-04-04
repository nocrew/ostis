#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "cprint.h"
#include "debug/debug.h"
#include "event.h"
#include "cpu.h"
#include "mfp.h"
#include "expr.h"
#include "shifter.h"
#include "glue.h"
#include "mmu.h"
#include "ikbd.h"
#include "instr.h"
#include "instr_clocked.h"
#include "state.h"
#include "diag.h"
#if TEST_BUILD
#include "tests/test_main.h"
#endif

#define PREVIOUS_INSTR_FINISHED(cpu) (cpu->instr_state < 0)

struct cpu *cpu;

struct cpubrk {
  struct cpubrk *next;
  LONG addr;
  int cnt;
};

struct cpuwatch {
  struct cpuwatch *next;
  char *cond;
  int cnt;
};

HANDLE_DIAGNOSTICS(cpu)

static int cpu_dec_breakpoint(LONG addr, int trace);
static int cpu_dec_watchpoint(int trace);
static struct cprint *default_instr_print(LONG addr, WORD op);
static struct cprint *illegal_instr_print(LONG addr, WORD op);

static struct cpubrk *brk = NULL;
static struct cpuwatch *watch = NULL;
static int interrupt_autovector[8];
static int exception_pending[256];
static int enter_debugger_after_instr = 0;
static int bus_error_flags = 0;
static int bus_error_address = 0;
static LONG last_pc = 0;
static int reset_cpu = 0;

int cprint_all = 0;

typedef void instr_t(struct cpu *, WORD);
static instr_t *instr[65536];
static instr_t *instr_clocked[65536];

void cpu_halt_for_debug() {
  cpu->debug_halted = 1;
}

static void default_instr(struct cpu *cpu, WORD op)
{
  printf("DEBUG: unknown opcode 0x%04x at 0x%08x\n", op, cpu->pc-2);
  cpu_print_status(CPU_USE_CURRENT_PC);
  mfp_print_status();
  if(!screen_check_disable()) {
    ENDLOOP;
  }
  exit(-3);

  if(0) default_instr(cpu, op);
}

static void illegal_instr(struct cpu *cpu, WORD op)
{
  cpu_set_exception(4);
}

static WORD fetch_instr(struct cpu *cpu)
{
  WORD op;
  last_pc = cpu->pc;
  if(cpu->has_prefetched) {
    cpu->has_prefetched = 0;
    cpu->pc += 2;
    return cpu->prefetched_instr;
  }
  op = bus_read_word(cpu->pc);
  cpu->pc += 2;
  return op;
}

static void cpu_exception_reset_sr()
{
  cpu_set_sr((cpu->sr|0x2000)&(~0x8000));
}

static void cpu_exception_full_stacked(int vnum)
{
  WORD oldsr;

  cpu_clear_prefetch();
  oldsr = cpu->sr;
  cpu_exception_reset_sr();
  cpu->a[7] -= 4;
  bus_write_long(cpu->a[7], cpu->pc);
  cpu->a[7] -= 2;
  bus_write_word(cpu->a[7], oldsr);
  cpu->a[7] -= 2;
  bus_write_word(cpu->a[7], bus_read_word(cpu->pc));
  cpu->a[7] -= 4;
  bus_write_long(cpu->a[7], bus_error_address);
  cpu->a[7] -= 2;
  bus_write_word(cpu->a[7], bus_error_flags);
  cpu->pc = bus_read_long(4*vnum);
  cpu_clr_exception(vnum);
  cpu_do_cycle(50);
}

static void cpu_exception_general(int vnum)
{
  WORD oldsr;

  cpu_clear_prefetch();
  oldsr = cpu->sr;
  cpu_exception_reset_sr();
  cpu->a[7] -= 4;
  bus_write_long(cpu->a[7], cpu->pc);
  cpu->a[7] -= 2;
  bus_write_word(cpu->a[7], oldsr);
  cpu_exception_reset_sr();
  cpu->pc = bus_read_long(4*vnum);
  cpu_clr_exception(vnum);
  cpu_do_cycle(34);
}

static void cpu_exception_interrupt(int vnum)
{
  WORD oldsr;

  oldsr = cpu->sr;
  int inum = vnum-IPL_EXCEPTION_VECTOR_OFFSET;
  if(inum <= IPL) { /* Do not run exception just yet. Leave it pending. */
    return;
  }
  cpu_clear_prefetch();
  cpu_exception_reset_sr();
  if(interrupt_autovector[inum] == IPL_NO_AUTOVECTOR) {
    cpu->a[7] -= 4;
    bus_write_long(cpu->a[7], cpu->pc);
    cpu->a[7] -= 2;
    bus_write_word(cpu->a[7], oldsr);
    cpu->sr = (cpu->sr&0xf0ff)|(inum<<8);
    cpu->pc = bus_read_long(4*vnum);
    cpu_clr_exception(vnum);
    cpu_do_cycle(48);
  } else {
    cpu->a[7] -= 4;
    bus_write_long(cpu->a[7], cpu->pc);
    cpu->a[7] -= 2;
    bus_write_word(cpu->a[7], oldsr);
    cpu_exception_reset_sr();
    cpu->sr = (cpu->sr&0xf0ff)|(inum<<8);
    cpu->pc = bus_read_long(4*interrupt_autovector[inum]);
    interrupt_autovector[inum] = -1;
    cpu_clr_exception(vnum);
    cpu_do_cycle(24);
  }
}

/* Called for each pending interrupt */
static void cpu_do_exception(int vnum)
{
  cpu->stopped = 0;

  if(vnum == 2 || vnum == 3) {
    cpu_exception_full_stacked(vnum);
  } else if(vnum == 4) {
    cpu_exception_general(4);
  } else if(vnum >= 25 && vnum <= 31) {
    cpu_exception_interrupt(vnum);
  } else {
    cpu_exception_general(vnum);
  }
}

static void cpu_do_reset(void)
{
  int i;

  reset_cpu = 0;
  cpu->pc = bus_read_long(4);
  cpu->sr = 0x2300;
  cpu->ssp = cpu->a[7] = bus_read_long(0);
  cpu->prefetched_instr = 0;
  cpu->has_prefetched = 0;
  cpu->ipl1 = 0;
  cpu->ipl2 = 0;

  for(i=0;i<256;i++) {
    exception_pending[i] = 0;
  }

  for(i=0;i<8;i++) {
    interrupt_autovector[i] = -1;
  }
}

int cpu_step_instr(int trace)
{
  WORD op;
  int vnum;

  if(cpu_dec_breakpoint(cpu->pc, trace)) return CPU_BREAKPOINT;
  if(cpu_dec_watchpoint(trace)) return CPU_WATCHPOINT;

  if(!cpu->stopped) {
#if TEST_BUILD
    test_call_hooks(TEST_HOOK_BEFORE_INSTR, cpu);
#endif
    if(cprint_all) {
      struct cprint *cprint;
      cprint = cprint_instr(cpu->pc);
      fprintf(stderr, "DEBUG-ASM: [%ld] %04x %06X      %s %s\n",
              cpu->cycle,
              cpu->sr,
              cpu->pc,
              cprint->instr,
              cprint->data);
      free(cprint);
    }

    op = fetch_instr(cpu);

    if(instr[op] == default_instr) {
      cpu->pc -= 2;
      return CPU_BREAKPOINT;
    }

    cpu->cyclecomp = 0;
    cpu->icycle = 0;

    if(cpu->sr&0x8000) {
      cpu_set_exception(VEC_TRACE);
    }
    
    instr[op](cpu, op);
#if TEST_BUILD
    test_call_hooks(TEST_HOOK_AFTER_INSTR, cpu);
#endif
  } else {
    instr[0x4e71](cpu, 0x4e71); /* Run NOP until STOP is cancelled */
  }
  cpu_do_cycle(cpu->icycle);
  cpu_check_for_pending_interrupts();

  /* If we get Bus Error, just ignore everything else */
  if(exception_pending[VEC_BUSERR]) {
    cpu_do_exception(VEC_BUSERR);
  } else if(exception_pending[VEC_ADDRERR]) {
    cpu_do_exception(VEC_ADDRERR);
  } else {
    /* We check for trace, interrupt, illegal and priv in that order,
       then any other exception,  but we trigger them in reverse order 
       because the last one runs first */

    /* Check TRAP */
    for(vnum=32;vnum<48;vnum++) {
      if(exception_pending[vnum]) { cpu_do_exception(vnum); }
    }
    
    if(exception_pending[VEC_ZERODIV]) { cpu_do_exception(VEC_ZERODIV); }
    if(exception_pending[VEC_CHK]) { cpu_do_exception(VEC_CHK); }
    if(exception_pending[VEC_TRAPV]) { cpu_do_exception(VEC_TRAPV); }
    if(exception_pending[VEC_LINEA]) { cpu_do_exception(VEC_LINEA); }
    if(exception_pending[VEC_LINEF]) { cpu_do_exception(VEC_LINEF); }
    if(exception_pending[VEC_PRIV]) { cpu_do_exception(VEC_PRIV); }
    if(exception_pending[VEC_ILLEGAL]) { cpu_do_exception(VEC_ILLEGAL); }    
    /* Check interrupts, in reverse order, to prevent lower IPLs to trigger */
    for(vnum=31;vnum>=25;vnum--) {
      if(exception_pending[vnum]) { cpu_do_exception(vnum); }
    }
    if(exception_pending[VEC_TRACE]) { cpu_do_exception(VEC_TRACE); }
  }

  if(enter_debugger_after_instr) {
    enter_debugger_after_instr = 0;
    return CPU_BREAKPOINT;
  }
    
  return CPU_OK;
}

int cpu_run(int cpu_run_state)
{
  static int counter = 1;
  int stop,ret;
  int tmp_run_state = CPU_RUN;

  /* If we're starting a run from the debugger, we'll pretend we're in
   * trace state for one step before retriggering breakpoints. This will
   * prevent the run feature in the debugger from immediately retriggering
   * the same breakpoint.
   */
  if(cpu_run_state == CPU_DEBUG_RUN) {
    tmp_run_state = CPU_TRACE;
  }

  stop = 0;

  event_init();

  while(!stop) {
    if(!cpu->debug_halted || cpu_run_state == CPU_DEBUG_RUN) {
      ret = cpu_step_instr(tmp_run_state);
      tmp_run_state = CPU_RUN;
    } else {
      ret = CPU_OK;
    }
    if(--counter < 0) {
      // Quick-and-somewhat-dirty solution to slow SDL polling.
      if(event_main() == EVENT_DEBUG) {
	stop = CPU_BREAKPOINT;
	break;
      }
      // This seems to be a good compromise between responsiveness and performance.
      counter = 1000;
    }
    if(ret != CPU_OK) {
      stop = ret;
      break;
    }
    if(reset_cpu) {
      cpu_do_reset();
    }
  }
  
  event_exit();
  return ret;
}

void cpu_reset_in(void)
{
  reset_cpu = 1;
}

void cpu_reset_out(void)
{
  // Call reset in just about every other device.
}

void cpu_do_cycle(LONG cnt)
{
  int i;

  if(cnt&3) {
    cnt = (cnt&0xfffffffc)+4;
  }
  // All intermediate operations inside an instruction take an even
  // amount of cycles.
  ASSERT((cnt & 1) == 0);

  cpu->clock = cpu->cycle;
  for(i = 0; i < cnt; i++) {
    glue_clock();
    mmu_clock();
    shifter_clock();
    cpu->clock++;
  }

  cpu->cycle += cnt;
}

void cpu_ipl1(void)
{
  cpu->ipl1 = 1;
}

void cpu_ipl2(void)
{
  cpu->ipl2 = 1;
}

void cpu_check_for_pending_interrupts()
{
  mmu_do_interrupts(cpu);

  if(cpu->ipl1 && (IPL < 2)) {
    cpu->ipl1 = 0;
    cpu_set_interrupt(IPL_HBL, IPL_NO_AUTOVECTOR);
  }
  if(cpu->ipl2 && (IPL < 4)) {
    cpu->ipl2 = 0;
    cpu_set_interrupt(IPL_VBL, IPL_NO_AUTOVECTOR);
  }
}

void cpu_add_extra_cycles(int cycle_count)
{
  if(!mmu_print_state) { /* Do not add cycles to icycle when we're just using the mmu for debug output */
    ADD_CYCLE(cycle_count);
  }
}

void cpu_set_interrupt(int ipl, int autovector)
{
  cpu_set_exception(ipl+IPL_EXCEPTION_VECTOR_OFFSET);
  interrupt_autovector[ipl] = autovector;
}

void cpu_set_exception(int vnum)
{
  exception_pending[vnum] = 1;
}

void cpu_clr_exception(int vnum)
{
  exception_pending[vnum] = 0;
}

int cpu_full_stacked_exception_pending()
{
  if(exception_pending[VEC_BUSERR] || exception_pending[VEC_ADDRERR]) {
    return 1;
  } else {
    return 0;
  }
}

static void cpu_set_full_stacked_exception(int vnum, int flags, LONG addr)
{
  bus_error_flags = flags;
  if((flags&0x08) == CPU_STACKFRAME_DATA) {
    bus_error_flags |= 0x1;
  } else {
    bus_error_flags |= 0x2;
  }
  if(cpu->sr&0x2000) {
    bus_error_flags |= 0x4;
  }
  bus_error_address = addr;
  exception_pending[vnum] = 1;
}

void cpu_set_bus_error(int flags, LONG addr)
{
  cpu_set_full_stacked_exception(VEC_BUSERR, flags, addr);
}

void cpu_set_address_error(int flags, LONG addr)
{
  cpu_set_full_stacked_exception(VEC_ADDRERR, flags, addr);
}

void cpu_set_sr(WORD sr)
{
  if((sr^cpu->sr)&0x2000) {
    if(cpu->sr&0x2000) {
      cpu->ssp = cpu->a[7];
      cpu->a[7] = cpu->usp;
    } else {
      cpu->usp = cpu->a[7];
      cpu->a[7] = cpu->ssp;
    }
  }

  cpu->sr = sr;
}

void cpu_prefetch()
{
  cpu->prefetched_instr = bus_read_word(cpu->pc);
  cpu->has_prefetched = 1;
}

void cpu_clear_prefetch()
{
  cpu->has_prefetched = 0;
}

void cpu_init()
{
  int i;

  cpu = xmalloc(sizeof(struct cpu));
  if(!cpu) {
    exit(-2);
  }

  cpu_do_reset();
  cpu->debug_halted = 0;
  cpu->cycle = 0;
  cpu->icycle = 0;
  cpu->stopped = 0;

  for(i=0;i<65536;i++) {
    instr[i] = default_instr;
    instr_print[i] = default_instr_print;
  }

  event_init();
  
  reset_init((void *)instr, (void *)instr_print);
  cmpi_init((void *)instr, (void *)instr_print);
  bcc_init((void *)instr, (void *)instr_print);

  sub_init((void *)instr, (void *)instr_print);
  suba_init((void *)instr, (void *)instr_print);
  subq_init((void *)instr, (void *)instr_print);
  subi_init((void *)instr, (void *)instr_print);
  subx_init((void *)instr, (void *)instr_print);
  sbcd_init((void *)instr, (void *)instr_print);

  lea_init((void *)instr, (void *)instr_print);
  pea_init((void *)instr, (void *)instr_print);

  jmp_init((void *)instr, (void *)instr_print);
  jsr_init((void *)instr, (void *)instr_print);

  bclr_init((void *)instr, (void *)instr_print);
  bset_init((void *)instr, (void *)instr_print);
  btst_init((void *)instr, (void *)instr_print);
  bchg_init((void *)instr, (void *)instr_print);

  scc_init((void *)instr, (void *)instr_print);
  dbcc_init((void *)instr, (void *)instr_print);

  clr_init((void *)instr, (void *)instr_print);
  cmpa_init((void *)instr, (void *)instr_print);

  asl_init((void *)instr, (void *)instr_print);
  asr_init((void *)instr, (void *)instr_print);
  lsr_init((void *)instr, (void *)instr_print);
  lsl_init((void *)instr, (void *)instr_print);
  ror_init((void *)instr, (void *)instr_print);
  rol_init((void *)instr, (void *)instr_print);
  roxl_init((void *)instr, (void *)instr_print);
  roxr_init((void *)instr, (void *)instr_print);

  add_init((void *)instr, (void *)instr_print);
  adda_init((void *)instr, (void *)instr_print);
  addq_init((void *)instr, (void *)instr_print);
  addi_init((void *)instr, (void *)instr_print);
  addx_init((void *)instr, (void *)instr_print);
  abcd_init((void *)instr, (void *)instr_print);

  move_init((void *)instr, (void *)instr_print);
  movea_init((void *)instr, (void *)instr_print); /* overlaps move_init */
  move_to_sr_init((void *)instr, (void *)instr_print);
  move_from_sr_init((void *)instr, (void *)instr_print);
  move_to_ccr_init((void *)instr, (void *)instr_print);
  moveq_init((void *)instr, (void *)instr_print);
  movep_init((void *)instr, (void *)instr_print); /* overlaps b{clr,set,tst}_init */
  movem_init((void *)instr, (void *)instr_print);
  move_usp_init((void *)instr, (void *)instr_print);
  
  rts_init((void *)instr, (void *)instr_print);
  rte_init((void *)instr, (void *)instr_print);
  rtr_init((void *)instr, (void *)instr_print);
  
  and_init((void *)instr, (void *)instr_print);
  exg_init((void *)instr, (void *)instr_print); /* overlaps and_init */
  andi_init((void *)instr, (void *)instr_print);
  andi_to_sr_init((void *)instr, (void *)instr_print); /* overlaps andi_init */
  andi_to_ccr_init((void *)instr, (void *)instr_print);
  
  or_init((void *)instr, (void *)instr_print);
  ori_init((void *)instr, (void *)instr_print);
  ori_to_sr_init((void *)instr, (void *)instr_print); /* overlaps ori_init */
  ori_to_ccr_init((void *)instr, (void *)instr_print); /*overlaps ori_init */

  eor_init((void *)instr, (void *)instr_print);
  eori_init((void *)instr, (void *)instr_print);
  eori_to_sr_init((void *)instr, (void *)instr_print); /* overlaps eori_init */
  eori_to_ccr_init((void *)instr, (void *)instr_print);

  cmp_init((void *)instr, (void *)instr_print); /* overlaps eor_init */
  cmpm_init((void *)instr, (void *)instr_print);

  trap_init((void *)instr, (void *)instr_print);

  tst_init((void *)instr, (void *)instr_print);
  tas_init((void *)instr, (void *)instr_print);

  mulu_init((void *)instr, (void *)instr_print);
  muls_init((void *)instr, (void *)instr_print);
  divu_init((void *)instr, (void *)instr_print);
  divs_init((void *)instr, (void *)instr_print);

  link_init((void *)instr, (void *)instr_print);
  unlk_init((void *)instr, (void *)instr_print);

  swap_init((void *)instr, (void *)instr_print);
  ext_init((void *)instr, (void *)instr_print); /* overlaps movem_init */

  nop_init((void *)instr, (void *)instr_print);
  linea_init((void *)instr, (void *)instr_print);
  linef_init((void *)instr, (void *)instr_print);

  not_init((void *)instr, (void *)instr_print);
  neg_init((void *)instr, (void *)instr_print);
  negx_init((void *)instr, (void *)instr_print);

  stop_init((void *)instr, (void *)instr_print);

  instr[0x4afc] = illegal_instr;
  instr_print[0x4afc] = illegal_instr_print;
  /* MOVEC, required for EmuTOS to even try to boot */
  instr[0x4e7a] = illegal_instr;
  instr_print[0x4e7a] = illegal_instr_print;
  instr[0x4e7b] = illegal_instr;
  instr_print[0x4e7b] = illegal_instr_print;

  instr[0x42c0] = illegal_instr;

#if TEST_BUILD
  test_cpu_init((void *)instr, (void *)instr_print);
#endif
}

void cpu_print_status(int which_pc)
{
  int i;
  LONG pc;

  pc = cpu->pc;
  
  if(which_pc == CPU_USE_LAST_PC) {
    pc = last_pc;
  }
  
  fprintf(stderr, "A:  ");
  for(i=0;i<4;i++)
    fprintf(stderr, "%08x ", cpu->a[i]);
  fprintf(stderr, "\nA:  ");
  for(i=4;i<8;i++)
    fprintf(stderr, "%08x ", cpu->a[i]);
  fprintf(stderr, "\n\n");
  fprintf(stderr, "D:  ");
  for(i=0;i<4;i++)
    fprintf(stderr, "%08x ", cpu->d[i]);
  fprintf(stderr, "\nD:  ");
  for(i=4;i<8;i++)
    fprintf(stderr, "%08x ", cpu->d[i]);
  fprintf(stderr, "\n");

  fprintf(stderr, "PC: %08x  USP: %08x  SSP: %08x\n", pc, cpu->usp, cpu->ssp);
  fprintf(stderr, "SR: %04x\n", cpu->sr);
  fprintf(stderr, "C:  %lld\n", (long long)cpu->cycle);
  fprintf(stderr, "BUS: %08x\n", bus_read_long(8));

  for(i=0;i<256;i++) {
    if(exception_pending[i]) {
      fprintf(stderr, "Pending exception: %d [%04x]\n", i, i*4);
    }
  }
}

static struct cprint *illegal_instr_print(LONG addr, WORD op)
{
  struct cprint *ret;
  
  ret = cprint_alloc(addr);
  strcpy(ret->instr, "ILLEGAL");
  sprintf(ret->data, "$%04x", op);

  return ret;
}

static struct cprint *default_instr_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);
  sprintf(ret->instr, "Unimplemented");
  sprintf(ret->data, "$%04x", op);

  return ret;
}

/* Debug related functions */

void cpu_print_breakpoints()
{
  struct cpubrk *t;
  
  t = brk;

  while(t) {
    printf("DEBUG: Breakpoint at 0x%x, count %d\n", t->addr, t->cnt);
    t = t->next;
  }
}

static struct cpubrk *cpu_find_breakpoint(LONG addr)
{
  struct cpubrk *t;
  
  t = brk;

  while(t) {
    if(t->addr == (addr&0xffffff)) return t;
    t = t->next;
  }
  
  return NULL;
}

int cpu_find_breakpoint_lowest_cnt(LONG addr)
{
  int lcnt;
  struct cpubrk *t;

  lcnt = -2;
  t = brk;
  
  while(t) {
    if(t->addr == (addr&0xffffff))
      if((lcnt == -2) || (t->cnt < lcnt))
	lcnt = t->cnt;
    t = t->next;
  }

  return (lcnt == -2)?0:lcnt;
}

static void cpu_remove_breakpoint(struct cpubrk *r)
{
  struct cpubrk *t;
  
  if(!r) return;
  if(brk == r) {
    brk = r->next;
    free(r);
    return;
  }

  t = brk;

  while(t) {
    if(t->next == r) {
      t->next = r->next;
      free(r);
      return;
    }
    t = t->next;
  }
}

static int cpu_dec_breakpoint(LONG addr, int trace)
{
  int trig;
  struct cpubrk *t,*l;
  
  t = brk;
  trig = 0;
  while(t) {
    if(t->addr == (addr&0xffffff)) {
      if(t->cnt != -1)
	t->cnt--;
      l = t->next;
      if(!t->cnt) {
	cpu_remove_breakpoint(t);
	trig = 1;
      } else if((t->cnt == -1) && (!trace)) {
	trig = 1;
      }
      t = l;
    } else {
      t = t->next;
    }
  }
  return trig;
}

int cpu_unset_breakpoint(LONG addr)
{
  int i;
  struct cpubrk *t;

  i = 0;

  t = cpu_find_breakpoint(addr);
  while(t) {
    cpu_remove_breakpoint(t);
    i++;
    t = cpu_find_breakpoint(addr);
  }
  if(i) return 0;
  return 1;
}

void cpu_set_breakpoint(LONG addr, int cnt)
{
  struct cpubrk *t;

  t = xmalloc(sizeof(struct cpubrk));
  t->addr = addr&0xffffff;
  t->cnt = cnt;
  t->next = brk;
  brk = t;
}

static void cpu_remove_watchpoint(struct cpuwatch *r)
{
  struct cpuwatch *t;
  
  if(!r) return;
  if(watch == r) {
    watch = r->next;
    free(r->cond);
    free(r);
    return;
  }

  t = watch;

  while(t) {
    if(t->next == r) {
      t->next = r->next;
      free(r->cond);
      free(r);
      return;
    }
    t = t->next;
  }
}

static int cpu_dec_watchpoint(int trace)
{
  int trig;
  struct cpuwatch *t,*l;
  LONG res;

  t = watch;
  trig = 0;
  while(t) {
    if(expr_parse(&res, t->cond) == EXPR_FAILURE) {
      cpu_remove_watchpoint(t);
      return 0;
    }
    if(res) {
      if(t->cnt != -1)
	t->cnt--;
      l = t->next;
      if(t->cnt == 0) {
	cpu_remove_watchpoint(t);
	trig = 1;
      } else if((t->cnt == -1) && (!trace)) {
	trig = 1;
      }
      t = l;
    } else {
      t = t->next;
    }
  }
  return trig;
}

void cpu_set_watchpoint(char *cond, int cnt)
{
  struct cpuwatch *t;
  
  t = xmalloc(sizeof(struct cpuwatch));
  t->cond = cond;
  t->cnt = cnt;
  t->next = watch;
  watch = t;
}

void cpu_enter_debugger()
{
  enter_debugger_after_instr = 1;
}



/* Functions for state collect/restore */

#define STATE_AREG    (0)
#define STATE_DREG    (STATE_AREG+4*8)
#define STATE_USP     (STATE_DREG+4*8)
#define STATE_SSP     (STATE_USP+4)
#define STATE_PC      (STATE_SSP+4)
#define STATE_SR      (STATE_PC+4)
#define STATE_CYCLE   (STATE_SR+2)
#define STATE_STOPPED (STATE_CYCLE+8)
#define STATE_INTPEND (STATE_STOPPED+4)
#define STATE_END     (STATE_INTPEND+4*8)

struct cpu_state *cpu_state_collect()
{
  struct cpu_state *new;
  int r;

  new = xmalloc(sizeof(struct cpu_state));
  if(new == NULL) return NULL;

  memcpy(new->id, "CPU0", 4);
  /* 
   * CPU state size:
   *
   * a[8]     == 4*8
   * d[8]     == 4*8
   * usp      == 4
   * ssp      == 4
   * pc       == 4
   * sr       == 2
   * cycle    == 8
   * stopped  == 4
   * int[8]   == 4*8 (pending interrupts)
   * ----------------
   * sum         
   * 
   */
  new->size = STATE_END;
  new->data = malloc(new->size);
  if(new->data == NULL) {
    free(new);
    return NULL;
  }
  for(r=0;r<8;r++) {
    state_write_mem_long(&new->data[STATE_AREG+r*4], cpu->a[r]);
  }
  for(r=0;r<8;r++) {
    state_write_mem_long(&new->data[STATE_DREG+r*4], cpu->d[r]);
  }
  state_write_mem_long(&new->data[STATE_USP], cpu->usp);
  state_write_mem_long(&new->data[STATE_SSP], cpu->ssp);
  state_write_mem_long(&new->data[STATE_PC], cpu->pc);
  state_write_mem_word(&new->data[STATE_SR], cpu->sr);
  state_write_mem_uint64(&new->data[STATE_CYCLE], cpu->cycle);
  state_write_mem_long(&new->data[STATE_STOPPED], cpu->stopped);
  //  for(r=0;r<8;r++) {
  //    state_write_mem_long(&new->data[STATE_INTPEND+r*4], interrupt_pending[r]);
  //  }

  return new;
}

void cpu_state_restore(struct cpu_state *state)
{
  int r;

  for(r=0;r<8;r++) {
    cpu->a[r] = state_read_mem_long(&state->data[STATE_AREG+r*4]);
  }
  for(r=0;r<8;r++) {
    cpu->d[r] = state_read_mem_long(&state->data[STATE_DREG+r*4]);
  }
  cpu->usp = state_read_mem_long(&state->data[STATE_USP]);
  cpu->ssp = state_read_mem_long(&state->data[STATE_SSP]);
  cpu->pc = state_read_mem_long(&state->data[STATE_PC]);
  cpu->sr = state_read_mem_word(&state->data[STATE_SR]);
  cpu->cycle = state_read_mem_uint64(&state->data[STATE_CYCLE]);
  cpu->stopped = state_read_mem_long(&state->data[STATE_STOPPED]);
  //  for(r=0;r<8;r++) {
  //    interrupt_pending[r] = state_read_mem_long(&state->data[STATE_INTPEND+r*4]);
  //  }
}


/* Clocked CPU functions */

/* There are overlaps right now, to keep the old code somewhat untouched. */
static void cpu_fetch_instr()
{
  cpu->current_instr = fetch_instr(cpu);
  cpu->icycle = 0;
  cpu->cyclecomp = 0;
}

/* The cycle delay for exceptions work slightly different to the old code,
 * so there are overlapping functions for these as well
 */

static void cpu_trigger_exception_full_stacked(int vnum)
{
  WORD oldsr;

  cpu_clear_prefetch();
  oldsr = cpu->sr;
  cpu_exception_reset_sr();
  cpu->a[7] -= 4;
  bus_write_long(cpu->a[7], cpu->pc);
  cpu->a[7] -= 2;
  bus_write_word(cpu->a[7], oldsr);
  cpu->a[7] -= 2;
  bus_write_word(cpu->a[7], bus_read_word(cpu->pc));
  cpu->a[7] -= 4;
  bus_write_long(cpu->a[7], bus_error_address);
  cpu->a[7] -= 2;
  bus_write_word(cpu->a[7], bus_error_flags);
  cpu->pc = bus_read_long(4*vnum);
  cpu_clr_exception(vnum);
  ADD_CYCLE(50);
}

static void cpu_trigger_exception_general(int vnum)
{
  WORD oldsr;

  cpu_clear_prefetch();
  oldsr = cpu->sr;
  cpu_exception_reset_sr();
  cpu->a[7] -= 4;
  bus_write_long(cpu->a[7], cpu->pc);
  cpu->a[7] -= 2;
  bus_write_word(cpu->a[7], oldsr);
  cpu_exception_reset_sr();
  cpu->pc = bus_read_long(4*vnum);
  cpu_clr_exception(vnum);
  ADD_CYCLE(34);
}

static void cpu_trigger_exception_interrupt(int vnum)
{
  WORD oldsr;

  oldsr = cpu->sr;
  int inum = vnum-IPL_EXCEPTION_VECTOR_OFFSET;
  if(inum <= IPL) { /* Do not run exception just yet. Leave it pending. */
    return;
  }
  cpu_clear_prefetch();
  cpu_exception_reset_sr();
  if(interrupt_autovector[inum] == IPL_NO_AUTOVECTOR) {
    cpu->a[7] -= 4;
    bus_write_long(cpu->a[7], cpu->pc);
    cpu->a[7] -= 2;
    bus_write_word(cpu->a[7], oldsr);
    cpu->sr = (cpu->sr&0xf0ff)|(inum<<8);
    cpu->pc = bus_read_long(4*vnum);
    cpu_clr_exception(vnum);
    ADD_CYCLE(48);
  } else {
    cpu->a[7] -= 4;
    bus_write_long(cpu->a[7], cpu->pc);
    cpu->a[7] -= 2;
    bus_write_word(cpu->a[7], oldsr);
    cpu_exception_reset_sr();
    cpu->sr = (cpu->sr&0xf0ff)|(inum<<8);
    cpu->pc = bus_read_long(4*interrupt_autovector[inum]);
    interrupt_autovector[inum] = -1;
    cpu_clr_exception(vnum);
    ADD_CYCLE(24);
  }
}

static void cpu_trigger_exception(int vnum)
{
  cpu->stopped = 0;

  if(vnum == 2 || vnum == 3) {
    cpu_trigger_exception_full_stacked(vnum);
  } else if(vnum == 4) {
    cpu_trigger_exception_general(4);
  } else if(vnum >= 25 && vnum <= 31) {
    cpu_trigger_exception_interrupt(vnum);
  } else {
    cpu_trigger_exception_general(vnum);
  }
}

static void cpu_trigger_exceptions()
{
  int vnum;
  
  cpu_check_for_pending_interrupts();

  /* If we get Bus Error, just ignore everything else. Same for Address Error */
  if(exception_pending[VEC_BUSERR]) {
    printf("DEBUG: Triggering BUS ERROR\n");
    cpu_trigger_exception(VEC_BUSERR);
  } else if(exception_pending[VEC_ADDRERR]) {
    printf("DEBUG: Triggering ADDRESS ERROR\n");
    cpu_trigger_exception(VEC_ADDRERR);
  } else {
    /* We check for trace, interrupt, illegal and priv in that order,
       then any other exception,  but we trigger them in reverse order 
       because the last one runs first */

    /* Check TRAP */
    for(vnum=32;vnum<48;vnum++) {
      if(exception_pending[vnum]) { cpu_trigger_exception(vnum); }
    }
    
    if(exception_pending[VEC_ZERODIV]) { cpu_trigger_exception(VEC_ZERODIV); }
    if(exception_pending[VEC_CHK]) { cpu_trigger_exception(VEC_CHK); }
    if(exception_pending[VEC_TRAPV]) { cpu_trigger_exception(VEC_TRAPV); }
    if(exception_pending[VEC_LINEA]) { cpu_trigger_exception(VEC_LINEA); }
    if(exception_pending[VEC_LINEF]) { cpu_trigger_exception(VEC_LINEF); }
    if(exception_pending[VEC_PRIV]) { cpu_trigger_exception(VEC_PRIV); }
    if(exception_pending[VEC_ILLEGAL]) { cpu_trigger_exception(VEC_ILLEGAL); }    
    /* Check interrupts, in reverse order, to prevent lower IPLs to trigger */
    for(vnum=31;vnum>=25;vnum--) {
      if(exception_pending[vnum]) { cpu_trigger_exception(vnum); }
    }
    if(exception_pending[VEC_TRACE]) { cpu_do_exception(VEC_TRACE); }
  }
}

static int cpu_new_instr(int cpu_run_state)
{
  cpu_trigger_exceptions();

  cpu->instr_state = INSTR_STATE_NONE;
  if(cprint_all) {
    struct cprint *cprint;
    cprint = cprint_instr(cpu->pc);
    fprintf(stderr, "DEBUG-ASM: [%ld] %04x %06X      %s %s\n",
            cpu->cycle,
            cpu->sr,
            cpu->pc,
            cprint->instr,
            cprint->data);
    free(cprint);
  }
  cpu_fetch_instr();
  return CPU_OK;
}

static int cpu_instr_done(int cpu_run_state)
{
  int ret;
  
  if(cpu_run_state == CPU_TRACE_SINGLE) {
    enter_debugger_after_instr = CPU_TRACE_SINGLE;
  } else if(cpu_dec_breakpoint(cpu->pc, cpu_run_state)) {
    enter_debugger_after_instr = CPU_BREAKPOINT;
  } else if(cpu_dec_watchpoint(cpu_run_state)) {
    enter_debugger_after_instr = CPU_WATCHPOINT;
  }

  if(enter_debugger_after_instr) {
    ret = enter_debugger_after_instr;
    enter_debugger_after_instr = 0;
    return ret;
  }
  return CPU_OK;
}

static int cpu_step_cycle(int cpu_run_state)
{
  int ret = CPU_OK;
  
  /* If icycle is positive, the previous instruction has not yet
   * used up all the cycles it's supposed to use, so decrement
   * the counter and exit
   */
  if(cpu->icycle > 0) {
    cpu->icycle--;
    return CPU_OK;
  }

  /* A new instruction will only start on multiple of 2 cycles, so if
   * that is not the case, exit and let it increment up to the proper point
   *
   * Until all instructions handle their own prefetch (of the next instruction)
   * completely as a state of its own, this needs to do 4c align, so while
   * this is a temporary revert, it is necessary until prefetch is done
   * the right way.
   */
  if((cpu->cycle&3) != 0) {
    return CPU_OK;
  }

  /* If the cpu is still stopped (STOP instruction) just check for exceptions,
   * or return without doing anything.
   */
  if(cpu->stopped) {
    cpu_trigger_exceptions();

    return CPU_OK;
  }

  /* INSTR_STATE_NONE means the instruction is not a state machine yet, and icycle is
   * the only indicator that an instruction has finished */
  if(cpu->instr_state == INSTR_STATE_NONE && cpu->icycle == 0) {
    cpu->instr_state = INSTR_STATE_FINISHED;
    return INSTR_STATE_FINISHED;
  }

  if(PREVIOUS_INSTR_FINISHED(cpu)) {
    /* Every instruction is initiated with the state NONE. If the instruction is a
     * properly clocked one, it will change the state as needed.
     * This is broken for now.
     */
    ret = cpu_new_instr(cpu_run_state);
  }

  /* Since the cpu struct includes the current instruction opcode now, passing 
   * it separately is somewhat unnecessary, and can be removed once the instructions
   * have been rewritten to be clocked
   */
  instr_clocked[cpu->current_instr](cpu, cpu->current_instr);

  /* This is a bit of hack until MOVEM is fixed. At the moment it reports 0 cycles
   * which would be bad to decrement.
   * The decrementation of 1 is necessary for the countdown to finished on 0 at the
   * right time
   */
  if(cpu->icycle > 0) {
    cpu->icycle--;
  }
  //  if(cpu->instr_state != INSTR_STATE_NONE) {
  //    cpu->icycle--;
  //  }

  /* When the clocked instruction is set to its finished state, this is returned, and
   * special End-of-instruction things can be applied
   */
  if(cpu->instr_state == INSTR_STATE_FINISHED) {
    ret = INSTR_STATE_FINISHED;
  }
  
  return ret;
}

/* cpu_clock() will be replacing cpu_step_cycle() when the 
 * master clock handling moves away from the cpu
 *
 * cpu_run_state is ether CPU_RUN which is the normal way, or
 * CPU_TRACE which does some extra checks to break out
 * into the debugger on breakpoints and such
 */
static int cpu_clock(int cpu_run_state)
{
  int ret;
  
  ret = cpu_step_cycle(cpu_run_state);

  /* INSTR_STATE_FINISHED should be returned whenever the last step of a cycle
   * was finishing the run of an instruction.
   * In this case the cycle counter is not updated. Instead there is one loop
   * to handle things where the code might return to the debugger */

  if(ret == INSTR_STATE_FINISHED && cpu->icycle == 0) {
    /* Check if an exception is in the way, and set things up to run that instead
     * of the next instruction.
     */
    return cpu_instr_done(cpu_run_state);
  } else {
    glue_clock();
    mmu_clock();
    shifter_clock();
    cpu->cycle++;
  }
  
  return ret;
}

/* Special function to step one instruction, used for single stepping debugger
 * This needs to be done in a better way really soon */
int cpu_step_instr_clocked(int cpu_run_state)
{
  int instr_finished = 0;
  int instr_finishing = 0;
  int ret;
  
  while(!instr_finished) {
    ret = cpu_clock(cpu_run_state);
    if(!instr_finishing && ret != CPU_OK) {
      instr_finishing = 1;
    }
    if(instr_finishing && cpu->icycle == 0) {
      instr_finished = 1;
    }
  }
  return CPU_TRACE_SINGLE;
}

/* The clocked equivalent version of cpu_run 
 * cpu_run_state is ether CPU_RUN which is the normal way, or
 * CPU_DEBUG_RUN which does some extra checks to break out
 * into the debugger on breakpoints and such
 */

int cpu_run_clocked(int cpu_run_state)
{
  static int counter = 1;
  int ret,stop;
  int tmp_run_state = CPU_RUN;

  /* If we're starting a run from the debugger, we'll pretend we're in
   * trace state for one step before retriggering breakpoints. This will
   * prevent the run feature in the debugger from immediately retriggering
   * the same breakpoint.
   */
  if(cpu_run_state == CPU_DEBUG_RUN) {
    tmp_run_state = CPU_TRACE;
  }

  stop = 0;
  event_init();

  while(!stop) {
    if(!cpu->debug_halted || cpu_run_state == CPU_DEBUG_RUN) {
      /* This skips the breakpoint on the current line when restarting from
       * the debugger, so that one doesn't get stuck on the same line all the time */
      ret = cpu_clock(tmp_run_state);
      tmp_run_state = CPU_RUN;
    } else {
      ret = CPU_OK;
    }

    /* SDL polling done every 8192 cycles or so */
    if(--counter < 0) {
      // Quick-and-somewhat-dirty solution to slow SDL polling.
      if(event_main() == EVENT_DEBUG) {
        enter_debugger_after_instr = 1;
      }
      // This seems to be a good compromise between responsiveness and performance.
      counter = 1000;
    }
    if(cpu_run_state == CPU_DEBUG_RUN &&
       ret != CPU_OK && ret != CPU_BREAKPOINT && ret != CPU_WATCHPOINT) {
      /* Ignore this, just to make sure the next condition is not triggered */
    } else if(ret != CPU_OK) {
      stop = ret;
      break;
    }
    if(reset_cpu) {
      cpu_do_reset();
    }
  }

  event_exit();
  return ret;
}

void cpu_init_clocked()
{
  int i;

  cpu = xmalloc(sizeof(struct cpu));
  if(!cpu) {
    exit(-2);
  }

  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(cpu, "CPU0");

  cpu_do_reset();
  cpu->debug_halted = 0;
  cpu->cycle = 0;
  cpu->icycle = 0;
  cpu->stopped = 0;
  cpu->instr_state = INSTR_STATE_NONE;

  for(i=0;i<65536;i++) {
    instr_clocked[i] = default_instr;
    instr_print[i] = default_instr_print;
  }

  event_init();
  
  reset_init((void *)instr_clocked, (void *)instr_print);
  cmpi_init((void *)instr_clocked, (void *)instr_print);
  bcc_instr_init((void *)instr_clocked, (void *)instr_print);

  sub_init((void *)instr_clocked, (void *)instr_print);
  suba_init((void *)instr_clocked, (void *)instr_print);
  subq_init((void *)instr_clocked, (void *)instr_print);
  subi_init((void *)instr_clocked, (void *)instr_print);
  subx_init((void *)instr_clocked, (void *)instr_print);
  sbcd_init((void *)instr_clocked, (void *)instr_print);

  lea_init((void *)instr_clocked, (void *)instr_print);
  pea_init((void *)instr_clocked, (void *)instr_print);

  jmp_init((void *)instr_clocked, (void *)instr_print);
  jsr_init((void *)instr_clocked, (void *)instr_print);

  bclr_init((void *)instr_clocked, (void *)instr_print);
  bset_init((void *)instr_clocked, (void *)instr_print);
  btst_init((void *)instr_clocked, (void *)instr_print);
  bchg_init((void *)instr_clocked, (void *)instr_print);

  scc_init((void *)instr_clocked, (void *)instr_print);
  dbcc_init((void *)instr_clocked, (void *)instr_print);

  clr_init((void *)instr_clocked, (void *)instr_print);
  cmpa_init((void *)instr_clocked, (void *)instr_print);

  asl_init((void *)instr_clocked, (void *)instr_print);
  asr_init((void *)instr_clocked, (void *)instr_print);
  lsr_init((void *)instr_clocked, (void *)instr_print);
  lsl_init((void *)instr_clocked, (void *)instr_print);
  ror_init((void *)instr_clocked, (void *)instr_print);
  rol_init((void *)instr_clocked, (void *)instr_print);
  roxl_init((void *)instr_clocked, (void *)instr_print);
  roxr_init((void *)instr_clocked, (void *)instr_print);

  add_init((void *)instr_clocked, (void *)instr_print);
  adda_init((void *)instr_clocked, (void *)instr_print);
  addq_init((void *)instr_clocked, (void *)instr_print);
  addi_init((void *)instr_clocked, (void *)instr_print);
  addx_init((void *)instr_clocked, (void *)instr_print);
  abcd_init((void *)instr_clocked, (void *)instr_print);

  move_init((void *)instr_clocked, (void *)instr_print);
  movea_init((void *)instr_clocked, (void *)instr_print); /* overlaps move_init */
  move_to_sr_init((void *)instr_clocked, (void *)instr_print);
  move_from_sr_init((void *)instr_clocked, (void *)instr_print);
  move_to_ccr_init((void *)instr_clocked, (void *)instr_print);
  moveq_init((void *)instr_clocked, (void *)instr_print);
  movep_init((void *)instr_clocked, (void *)instr_print); /* overlaps b{clr,set,tst}_init */
  movem_instr_init((void *)instr_clocked, (void *)instr_print);
  move_usp_init((void *)instr_clocked, (void *)instr_print);
  
  rts_init((void *)instr_clocked, (void *)instr_print);
  rte_init((void *)instr_clocked, (void *)instr_print);
  rtr_init((void *)instr_clocked, (void *)instr_print);
  
  and_init((void *)instr_clocked, (void *)instr_print);
  exg_instr_init((void *)instr_clocked, (void *)instr_print); /* overlaps and_init */
  andi_init((void *)instr_clocked, (void *)instr_print);
  andi_to_sr_init((void *)instr_clocked, (void *)instr_print); /* overlaps andi_init */
  andi_to_ccr_init((void *)instr_clocked, (void *)instr_print);
  
  or_init((void *)instr_clocked, (void *)instr_print);
  ori_init((void *)instr_clocked, (void *)instr_print);
  ori_to_sr_init((void *)instr_clocked, (void *)instr_print); /* overlaps ori_init */
  ori_to_ccr_init((void *)instr_clocked, (void *)instr_print); /*overlaps ori_init */

  eor_init((void *)instr_clocked, (void *)instr_print);
  eori_init((void *)instr_clocked, (void *)instr_print);
  eori_to_sr_init((void *)instr_clocked, (void *)instr_print); /* overlaps eori_init */
  eori_to_ccr_init((void *)instr_clocked, (void *)instr_print);

  cmp_init((void *)instr_clocked, (void *)instr_print); /* overlaps eor_init */
  cmpm_init((void *)instr_clocked, (void *)instr_print);

  trap_init((void *)instr_clocked, (void *)instr_print);

  tst_init((void *)instr_clocked, (void *)instr_print);
  tas_init((void *)instr_clocked, (void *)instr_print);

  mulu_init((void *)instr_clocked, (void *)instr_print);
  muls_init((void *)instr_clocked, (void *)instr_print);
  divu_init((void *)instr_clocked, (void *)instr_print);
  divs_init((void *)instr_clocked, (void *)instr_print);

  link_init((void *)instr_clocked, (void *)instr_print);
  unlk_init((void *)instr_clocked, (void *)instr_print);

  swap_init((void *)instr_clocked, (void *)instr_print);
  ext_init((void *)instr_clocked, (void *)instr_print); /* overlaps movem_init */

  nop_init((void *)instr_clocked, (void *)instr_print);
  linea_init((void *)instr_clocked, (void *)instr_print);
  linef_init((void *)instr_clocked, (void *)instr_print);

  not_init((void *)instr_clocked, (void *)instr_print);
  neg_init((void *)instr_clocked, (void *)instr_print);
  negx_init((void *)instr_clocked, (void *)instr_print);

  stop_init((void *)instr_clocked, (void *)instr_print);

  instr_clocked[0x4afc] = illegal_instr;
  instr_print[0x4afc] = illegal_instr_print;
  /* MOVEC, required for EmuTOS to even try to boot */
  instr_clocked[0x4e7a] = illegal_instr;
  instr_print[0x4e7a] = illegal_instr_print;
  instr_clocked[0x4e7b] = illegal_instr;
  instr_print[0x4e7b] = illegal_instr_print;

  instr_clocked[0x42c0] = illegal_instr;

#if TEST_BUILD
  test_cpu_init((void *)instr_clocked, (void *)instr_print);
#endif
}

