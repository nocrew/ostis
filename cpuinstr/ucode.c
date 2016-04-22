#include "common.h"
#include "ucode.h"

#define INSTR_STATE_RUNNING 1

static u_sequence *seq_pointer;

static int ucycles;
static uop_t *upc;

void ujump(uop_t *uop, int cyles)
{
  upc = uop;
  ucycles = cyles;
}

int ustep(void)
{
  if(ucycles == 0)
    return 1;

  ucycles--;
  (*upc)();
  upc++;
  
  return 0;
}

void u_start_sequence(u_sequence *seq, struct cpu *cpu, WORD op)
{
  if(cpu->instr_state == INSTR_STATE_NONE) {
    cpu->instr_state = INSTR_STATE_RUNNING;
    seq_pointer = seq;
    ujump(0, 0);
  }

  while(ustep()) {
    (*seq_pointer)(cpu, op);
    seq_pointer++;
    if(cpu->instr_state == INSTR_STATE_FINISHED)
      break;
    else
      ADD_CYCLE(2);
  }    
}

void u_end_sequence(struct cpu *cpu, WORD op)
{
  cpu->instr_state = INSTR_STATE_FINISHED;
  ujump(0, 0);
}

// Do nothing.
static void u_nop(void)
{
}

// Fetch from program.
static void u_pf(void)
{
  cpu_prefetch();
}

uop_t nop_uops[] = { u_nop, u_nop };
static uop_t prefetch_uops[] = { u_pf, u_nop };

void u_prefetch(struct cpu *cpu, WORD op)
{
  ujump(prefetch_uops, 2);
}
