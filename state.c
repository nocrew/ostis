#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"
#include "cpu.h"
#include "state.h"

static long lastid = 0;

void state_write_mem_byte(char *ptr, BYTE data)
{
  ptr[0] = data;
}

void state_write_mem_word(char *ptr, WORD data)
{
  ptr[0] = data>>8;
  ptr[1] = data;
}

void state_write_mem_long(char *ptr, LONG data)
{
  ptr[0] = data>>24;
  ptr[1] = data>>16;
  ptr[2] = data>>8;
  ptr[3] = data;
}

BYTE state_read_mem_byte(char *ptr)
{
  return ptr[0];
}

WORD state_read_mem_word(char *ptr)
{
  return (ptr[0]<<8)|ptr[1];
}

LONG state_read_mem_long(char *ptr)
{
  return (ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|ptr[3];
}

static void state_write_file_long(FILE *f, long data)
{
  fprintf(f, "%c", (unsigned char)(data>>24));
  fprintf(f, "%c", (unsigned char)(data>>16));
  fprintf(f, "%c", (unsigned char)(data>>8));
  fprintf(f, "%c", (unsigned char)(data));
}

static void state_write_section_header(FILE *f, char *id, unsigned long size)
{
  fwrite(id, 4, 1, f);

  /* size is always saved in big endian */
  state_write_file_long(f, size);
}

static void state_write_section(FILE *f, char *id, long size, char *data)
{
  char dummy[4] = {0, 0, 0, 0};

  state_write_section_header(f, id, size);
  fwrite(data, size, 1, f);
  if(size%4) /* Align to 32bit */
    fwrite(dummy, 4-(size%4), 1, f); 
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

  lastid++;
  new->size = new->cpu_state->size + state_get_mmu_size(new->mmu_state);
  new->id = lastid;
  new->version = STATE_VERSION;

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
  state_write_section_header(f, "STAT", state->size);
  state_write_section_header(f, "STID", 4);
  state_write_file_long(f, state->id);
  state_write_section_header(f, "STVE", 4);
  state_write_file_long(f, state->version);

  state_write_section(f, state->cpu_state->id, 
		      state->cpu_state->size,
		      state->cpu_state->data);

  t = state->mmu_state;
  while(t) {
    state_write_section(f, t->id, t->size, t->data);
    t = t->next;
  }
  fclose(f);
}

struct state *state_load(char *filename)
{
  struct state *new;
  
  new = (struct state *)malloc(sizeof(struct state));
  if(new == NULL) return NULL;
  
  return NULL;
}

int state_valid_id(char *id)
{
  if(!strcmp("STAT", id) ||  /* File header */
     !strcmp("STID", id) ||  /* State ID (32bit big endian value) */
     !strcmp("STVE", id) ||  /* State Version (32bit big endian value) */
     !strcmp("STTS", id) ||  /* State TS (64bit big endian cycle count) */
     !strcmp("STDL", id) ||  /* State Delta Reference ID
				(32bit big endian value) */
     !strcmp("CPU0", id)) {  /* CPU State */
    return STATE_INVALID;
  }

  return STATE_VALID;
}

