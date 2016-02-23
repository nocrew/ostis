#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "debug/debug.h"
#include "event.h"
#include "cpu.h"
#include "mfp.h"
#include "expr.h"
#include "shifter.h"
#include "mmu.h"
#include "ikbd.h"
#include "instr.h"
#include "state.h"

#define EXTREME_EXCEPTION_DEBUG 0

struct cpu *cpu;

struct cpudbg {
  struct cpudbg *next;
  LONG addr;
};

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

static struct cpudbg *dbg = NULL;
static struct cprint_label *clabel = NULL;
static struct cpubrk *brk = NULL;
static struct cpuwatch *watch = NULL;
static int interrupt_pending[8];
static int interrupt_autovector = -1;
static int enter_debugger_after_instr = 0;

#if EXTREME_EXCEPTION_DEBUG == 2
int cprint_all = 1;
#else
int cprint_all;
#endif

typedef void instr_t(struct cpu *, WORD);
static instr_t *instr[65536];

typedef struct cprint *instr_print_t(LONG, WORD);
static instr_print_t *instr_print[65536];

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

  new = (struct cpu_state *)malloc(sizeof(struct cpu_state));
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
  new->data = (char *)malloc(new->size);
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
  for(r=0;r<8;r++) {
    state_write_mem_long(&new->data[STATE_INTPEND+r*4], interrupt_pending[r]);
  }

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
  for(r=0;r<8;r++) {
    interrupt_pending[r] = state_read_mem_long(&state->data[STATE_INTPEND+r*4]);
  }
}

void cpu_halt_for_debug() {
  cpu->debug_halted = 1;
}

static void default_instr(struct cpu *cpu, WORD op)
{
  printf("DEBUG: unknown opcode 0x%04x at 0x%08x\n", op, cpu->pc-2);
  cpu_print_status();
  mfp_print_status();
  //  shifter_print_status();
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

static WORD fetch_instr(struct cpu *cpu)
{
  WORD op;
  op = mmu_read_word(cpu->pc);
  cpu->pc += 2;
  return op;
}

static void cpu_debug_check(LONG addr)
{
  struct cpudbg *t;
  t = dbg;
  while(t) {
    if((addr&0xffffff) == t->addr) {
      cpu->debug = 1;
      return;
    }
    t = t->next;
  }
}

static void cpu_do_exception(int vnum)
{
  WORD oldsr;
#if EXTREME_EXCEPTION_DEBUG
  LONG oldpc;
#endif
  int i;
  oldsr = cpu->sr;
  cpu_set_sr((cpu->sr|0x2000)&(~0x8000)); /* set supervisor, clear trace */
  fflush(stdout);

  cpu->stopped = 0;

  //  if(vnum > 0) {
  //    printf("DEBUG: Do exception: %d [%04x] (PC == %08x)\n", vnum, vnum * 4, cpu->pc);
  //  }

  if(vnum == 2) {
#if EXTREME_EXCEPTION_DEBUG
    printf("DEBUG: %d Entering BUS ERROR\n", cpu->cycle);
#endif
    //    cprint_all = 0;
    if(cpu->debug) {
      printf("DEBUG: Entering Bus Error\n");
    }
    cpu->a[7] -= 14;
    mmu_write_word(cpu->a[7], oldsr);
    cpu->pc = mmu_read_long(4*vnum);
    cpu_do_cycle(50, 0);
  } else if(vnum == 4) {
#if EXTREME_EXCEPTION_DEBUG
  printf("DEBUG: %d Entering ILLEGAL INSTRUCTION\n", cpu->cycle);
#endif
    printf("ILLEGAL Instruction\n");
    cpu->a[7] -= 4;
    mmu_write_long(cpu->a[7], cpu->pc);
    cpu->a[7] -= 2;
    mmu_write_word(cpu->a[7], oldsr);
    cpu->pc = mmu_read_long(4*vnum);
    cpu_do_cycle(34, 0);
  } else if(vnum >= 25 && vnum <= 31) {
    i = vnum-24;
    if((interrupt_pending[i] >= 0) && (IPL < i)) {
      if(interrupt_autovector == IPL_NO_AUTOVECTOR) {
        //        printf("DEBUG: Do interrupt: %d [IPL: %d] (PC == %08x)\n", i, IPL, cpu->pc);
        interrupt_pending[i] = 0;
        cpu->a[7] -= 4;
        mmu_write_long(cpu->a[7], cpu->pc);
        cpu->a[7] -= 2;
        mmu_write_word(cpu->a[7], oldsr);
#if EXTREME_EXCEPTION_DEBUG
        oldpc = cpu->pc;
#endif
        cpu->pc = mmu_read_long(4*(i+24));
#if EXTREME_EXCEPTION_DEBUG
        printf("DEBUG: %d [%04x] [%08x => %08x] Entering interrupt %d [%02x]\n", cpu->cycle, cpu->sr, oldpc, cpu->pc, vnum, vnum * 4);
#endif
        cpu_do_cycle(48, 0);
        cpu->sr = (cpu->sr&0xf0ff)|(i<<8);
      } else {
        //        printf("DEBUG: Do interrupt: %d [IPL: %d] Autovector to %08x (PC == %08x)\n", i, IPL, interrupt_autovector, cpu->pc);
        cpu->a[7] -= 4;
        mmu_write_long(cpu->a[7], cpu->pc);
        cpu->a[7] -= 2;
        mmu_write_word(cpu->a[7], oldsr);
#if EXTREME_EXCEPTION_DEBUG
        oldpc = cpu->pc;
#endif
        cpu->pc = mmu_read_long(4*interrupt_autovector);
#if EXTREME_EXCEPTION_DEBUG
        printf("DEBUG: %d [%04x] [%08x => %08x] Entering interrupt %d [%02x] (autovector: %d [%04x])\n", cpu->cycle, cpu->sr, oldpc, cpu->pc, vnum, vnum * 4, interrupt_autovector, interrupt_autovector * 4);
#endif
        cpu->sr = (cpu->sr&0xf0ff)|(i<<8);
        cpu_do_cycle(24, 0); /* From Hatari */
      }
    } else {
      cpu->sr = oldsr;
#if EXTREME_EXCEPTION_DEBUG
      if(IPL < 7) {
        printf("DEBUG: Missed interrupt %d [%02x] (autovector: %02x) - %04x\n", vnum, vnum * 4, interrupt_autovector, cpu->sr);
      }
#endif
      //      printf("Missed interrupt: %d, IPL == %d\n", i, IPL);
    }
  } else {
    //    if(vnum > 25) return;
    cpu->a[7] -= 4;
    mmu_write_long(cpu->a[7], cpu->pc);
    cpu->a[7] -= 2;
    mmu_write_word(cpu->a[7], oldsr);
#if EXTREME_EXCEPTION_DEBUG
    oldpc = cpu->pc;
#endif
    cpu->pc = mmu_read_long(4*vnum);
#if EXTREME_EXCEPTION_DEBUG
    printf("DEBUG: %d [%04x] [%08x => %08x] Entering other %d [%02x]\n", cpu->cycle, cpu->sr, oldpc, cpu->pc, vnum, vnum * 4);
#endif
    cpu_do_cycle(34, 0);
  }
}

void cpu_add_debugpoint(LONG addr)
{
  struct cpudbg *t;
  
  t = (struct cpudbg *)malloc(sizeof(struct cpudbg));
  t->addr = addr&0xffffff;
  t->next = dbg;
  dbg = t;
}

struct cprint *cprint_instr(LONG addr)
{
  WORD op;

  op = mmu_read_word_print(addr);
  
  return instr_print[op](addr, op);
}

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

  t = (struct cpubrk *)malloc(sizeof(struct cpubrk));
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
  
  t = (struct cpuwatch *)malloc(sizeof(struct cpuwatch));
  t->cond = cond;
  t->cnt = cnt;
  t->next = watch;
  watch = t;
}

void cpu_enter_debugger()
{
  enter_debugger_after_instr = 1;
}

int cpu_step_instr(int trace)
{
#if 0
  int i;
#endif
  WORD op;

#if 1
  if(cpu_dec_breakpoint(cpu->pc, trace)) return CPU_BREAKPOINT;
  if(cpu_dec_watchpoint(trace)) return CPU_WATCHPOINT;

  cpu_debug_check(cpu->pc);
  if(cpu->debug) {
    if(cpu->pc < 0xfc0000)
      printf("DEBUG: PC == 0x%x\n", cpu->pc);
    //    mfp_print_status();
  }
#endif

  cpu->exception_pending = -1;
#if 0
  for(i=0;i<8;i++) {
    if(interrupt_pending[i]) {
      cpu->exception_pending = -2;
      break;
    }
  }
#else
#if 0
  if(interrupt_pending[2]) {
    cpu->exception_pending = 2+24;
  } else if(interrupt_pending[4]) {
    cpu->exception_pending = 4+24;
  }
#endif
#endif

#if 0
  if(cpu->pc == 0xf26) {
    printf("DEBUG: [a1] == %08x\n", mmu_read_long(cpu->a[1]));
  }
  if(cpu->pc == 0xf28) {
    printf("DEBUG: [a1] == %08x\n", mmu_read_long(cpu->a[1]-4));
  }
#endif

  if(!cpu->stopped) {
    op = fetch_instr(cpu);

#if 1
    if(instr[op] == default_instr) {
      cpu->pc -= 2;
      return CPU_BREAKPOINT;
    }
#endif

    cpu->cyclecomp = 0;
    cpu->icycle = 0;
    cpu->tracedelay = 0;

    if(cprint_all) {
      struct cprint *cprint;
      cprint = cprint_instr(cpu->pc-2);
      printf("DEBUG-ASM: %04x %02x (%d) %06X      %s %s",
             cpu->sr,
             mfp_get_ISRB(),
             ikbd_get_fifocnt(),
             cpu->pc-2,
             cprint->instr,
             cprint->data);
      free(cprint);
    }
    instr[op](cpu, op);
  } else {
    instr[0x4e71](cpu, 0x4e71); /* Run NOP until STOP is cancelled */
  }
  if(cprint_all) {
    int cnt = cpu->icycle;
    if(cnt&3) {
      cnt = (cnt&0xfffffffc)+4;
    }
    printf("    [%d => %d]\n", cpu->icycle, cnt);
  }
  cpu_do_cycle(cpu->icycle, 0);

  if(cpu->exception_pending != -1) {
    cpu_do_exception(cpu->exception_pending);
  }
  if(CHKT && !cpu->tracedelay) cpu_do_exception(9);
  if(enter_debugger_after_instr) {
    enter_debugger_after_instr = 0;
    return CPU_BREAKPOINT;
  }
    
  return CPU_OK;
}

int cpu_run(int cpu_run_state)
{
  int stop,ret;

  stop = 0;

  event_init();

  while(!stop) {
    if(!cpu->debug_halted || cpu_run_state == CPU_DEBUG_RUN) {
      ret = cpu_step_instr(CPU_RUN);
    } else {
      ret = CPU_OK;
    }
    if(event_main() == EVENT_DEBUG) {
      stop = CPU_BREAKPOINT;
      break;
    }
    if(ret != CPU_OK) {
      stop = ret;
      break;
    }
  }
  
  event_exit();
  return ret;
}

void cpu_print_status()
{
  int i;
  
  printf("A:  ");
  for(i=0;i<4;i++)
    printf("%08x ", cpu->a[i]);
  printf("\nA:  ");
  for(i=4;i<8;i++)
    printf("%08x ", cpu->a[i]);
  printf("\n\n");
  printf("D:  ");
  for(i=0;i<4;i++)
    printf("%08x ", cpu->d[i]);
  printf("\nD:  ");
  for(i=4;i<8;i++)
    printf("%08x ", cpu->d[i]);
  printf("\n");

  printf("PC: %08x  USP: %08x  SSP: %08x\n", cpu->pc, cpu->usp, cpu->ssp);
  printf("SR: %04x\n", cpu->sr);
  printf("C:  %ld\n", cpu->cycle);
  printf("BUS: %08x\n", mmu_read_long(8));
}

void cpu_do_cycle(LONG cnt, int noint)
{
  /* 73574 */
  if(cnt&3) {
    cnt = (cnt&0xfffffffc)+4;
  }
  cpu->cycle += cnt;
  if(cpu->no_exceptions) return;
  shifter_do_interrupts(cpu, noint);
  if(noint) return;

  mmu_do_interrupts(cpu);
}

void cpu_set_interrupt(int ipl, int autovector)
{
  interrupt_pending[ipl] = 1;
  cpu->exception_pending = ipl + 24;
  interrupt_autovector = autovector;
}

void cpu_set_exception(int vnum)
{
  if((vnum >= 25) && (vnum <= 31)) {
    printf("DEBUG: Got exception instead of interrupt: %d\n", vnum-24);
    exit(-99);
    interrupt_pending[vnum-24] = 1;
    cpu->exception_pending = -2;
  } else {
    cpu->exception_pending = vnum;
  }
}

void cpu_set_sr(WORD sr)
{
  if(sr & 0x8000) {
    printf("DEBUG: Setting trace bit in SR\n");
  }
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

int cprint_label_exists(char *name)
{
  struct cprint_label *t;
  
  t = clabel;

  while(t) {
    if(!strcasecmp(t->name, name)) return 1;
    t = t->next;
  }
  
  return 0;
}

LONG cprint_label_addr(char *name)
{
  struct cprint_label *t;
  
  t = clabel;
  
  while(t) {
    if(!strcasecmp(t->name, name)) return t->addr;
    t = t->next;
  }
  
  return 0;
}

char *cprint_find_label(LONG addr)
{
  struct cprint_label *t;

  t = clabel;

  while(t) {
    if(t->named && (t->addr == (addr&0xffffff))) return t->name;
    t = t->next;
  }

  t = clabel;

  while(t) {
    if(t->addr == (addr&0xffffff)) return t->name;
    t = t->next;
  }

  return NULL;
}

static char *cprint_find_auto_label(LONG addr)
{
  struct cprint_label *t;

  t = clabel;

  while(t) {
    if(!t->named && (t->addr == (addr&0xffffff))) return t->name;
    t = t->next;
  }

  return NULL;
}

void cprint_set_label(LONG addr, char *name)
{
  struct cprint_label *new;
  static char tname[10];
  char *tmp;

  if(!debugger)
    return;

  if(name) {
    tmp = cprint_find_label(addr);
    if(tmp && !strcmp(tmp, name)) return;
  } else {
    tmp = cprint_find_auto_label(addr);
    sprintf(tname, "L_%x", addr&0xffffff);
    if(tmp && !strcmp(tmp, tname)) return;
  }

  new = (struct cprint_label *)malloc(sizeof(struct cprint_label));
  new->addr = addr&0xffffff;
  if(name) {
    new->name = name;
    new->named = 1;
  } else {
    new->name = (char *)malloc(10);
    sprintf(new->name, "L_%x", addr&0xffffff);
    new->named = 0;
  }
  new->next = clabel;
  clabel = new;
}

void cprint_save_labels(char *file)
{
  FILE *f;
  struct cprint_label *t;

  f = fopen(file, "wct");
  if(!f) {
    perror("fopen");
  }
  
  t = clabel;

  while(t) {
    if(t->named) {
      fprintf(f, "%-14s %08X\n", t->name, t->addr);
    }
    t = t->next;
  }
  fclose(f);
}

void cprint_load_labels(char *file)
{
  FILE *f;
  char name[1024];
  LONG addr;

  f = fopen(file, "rb");
  if(!f) {
    perror("fopen");
  }

  while(!feof(f)) {
    if(fscanf(f, "%14s %08X\n", name, &addr) != 2)
      WARNING(fscanf);
    cprint_set_label(addr, strdup(name));
  }
  
  fclose(f);
}

void cpu_init()
{
  int i;

  cpu = (struct cpu *)malloc(sizeof(struct cpu));
  if(!cpu) {
    exit(-2);
  }

  cpu->pc = mmu_read_long(4);
  cpu->sr = 0x2300;
  cpu->ssp = cpu->a[7] = mmu_read_long(0);
  cpu->debug = 0;
  cpu->debug_halted = 0;
  cpu->a[5] = 0xdeadbeef;
  cpu->exception_pending = -1;
  cpu->no_exceptions = 0;
  cpu->cycle = 0;
  cpu->stopped = 0;

  for(i=0;i<65536;i++) {
    instr[i] = default_instr;
    instr_print[i] = default_instr_print;
    //    instr[i] = illegal_instr;
    //    instr_print[i] = illegal_instr_print;
  }

  for(i=0;i<8;i++) {
    interrupt_pending[i] = 0;
  }

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
  instr[0x42c0] = illegal_instr;
}
