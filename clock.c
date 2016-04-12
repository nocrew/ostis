#include "clock.h"
#include "glue.h"
#include "mmu.h"
#include "shifter.h"

struct delay {
  unsigned clock;
  void (*callback)(void);
  struct delay *next;
};

static struct delay *first_delay = NULL;

void clock_step(void)
{
  while(first_delay != NULL && first_delay->clock == 0) {
    first_delay->callback();
    free(first_delay);
    first_delay = first_delay->next;
  }
  if(first_delay != NULL)
    first_delay->clock--;

  glue_clock();
  mmu_clock();
  shifter_clock();
}

static void insert(struct delay **list, struct delay *elt)
{
  if(*list == NULL)
    *list = elt;
  else if(elt->clock < (*list)->clock) {
    elt->next = *list;
    *list = elt;
    elt->next->clock -= elt->clock;
  } else {
    elt->clock -= (*list)->clock;
    insert(&(*list)->next, elt);
  }
}

void clock_delay(unsigned cycles, void (*callback)(void))
{
  struct delay *d = xmalloc(sizeof(struct delay));
  d->clock = cycles;
  d->callback = callback;
  d->next = NULL;
  insert(&first_delay, d);
}

