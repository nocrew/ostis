#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"
#include "cpu.h"
#include "state.h"

static void state_write_section_header(FILE *f, char *id, unsigned long size)
{
  fwrite(id, 4, 1, f);

  /* size is always saved in big endian */
  fprintf(f, "%c", (unsigned char)(size>>24));
  fprintf(f, "%c", (unsigned char)(size>>16));
  fprintf(f, "%c", (unsigned char)(size>>8));
  fprintf(f, "%c", (unsigned char)(size));
}

static void state_write_section(FILE *f, char *id, long size, char *data)
{
  state_write_section_header(f, id, size);
  fwrite(data, size, 1, f);
}

static void state_clear_mmu(struct mmu_state *state)
{
  while(state) {
    free(state->data);
    state = state->next;
  }
}

static long state_get_mmu_size(struct mmu_state *state)
{
  long size = 0;

  while(state) {
    size += state->size;
    state = state->next;
  }
  return size;
}

struct state *state_collect()
{
  struct state *new;

  new = (struct state *)malloc(sizeof(struct state));

  if(new == NULL) return NULL;

  strcpy(new->id, "STAT");
  new->cpu_state = cpu_state_collect();
  if(new->cpu_state == NULL) {
    free(new);
    return NULL;
  }
  new->mmu_state = mmu_state_collect();
  if(new->mmu_state == NULL) {
    free(new->cpu_state->data);
    free(new);
    return NULL;
  }

  new->size = new->cpu_state->size + state_get_mmu_size(new->mmu_state);

  return new;
}

void state_restore(struct state *state)
{
  cpu_state_restore(state->cpu_state);
  mmu_state_restore(state->mmu_state);
}

void state_clear(struct state *state)
{
  if(state == NULL) return;

  if(state->cpu_state) free(state->cpu_state->data);
  if(state->mmu_state) state_clear_mmu(state->mmu_state);
  free(state);
}

void state_save(char *filename, struct state *state)
{
  struct mmu_state *t;
  FILE *f;

  f = fopen(filename, "wct");
  state_write_section_header(f, state->id, state->size);

  state_write_section(f, state->cpu_state->id, 
		      state->cpu_state->size,
		      state->cpu_state->data);

  t = state->mmu_state;
  while(t) {
    state_write_section(f, t->id, t->size, t->data);
    t = t->next;
  }
}

struct state *state_load(char *filename)
{
  struct state *new;

  new = (struct state *)malloc(sizeof(struct state));
  if(new == NULL) return NULL;

  return NULL;
}
