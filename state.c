#include <stdio.h>
#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mmu.h"
#include "cpu.h"
#include "state.h"

static struct state_list *list = NULL;

static long maxid = 0;
struct state_section {
  char id[4];
  long size;
  char *data;
};

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

void state_write_mem_uint64(char *ptr, uint64_t data)
{
  ptr[0] = data>>56;
  ptr[1] = data>>48;
  ptr[2] = data>>40;
  ptr[3] = data>>32;
  ptr[4] = data>>24;
  ptr[5] = data>>16;
  ptr[6] = data>>8;
  ptr[7] = data;
}

void state_write_mem_ptr(char *ptr, void *data)
{
  intptr_t x = (intptr_t)data;
  int i;
  for(i = 0; i < sizeof data; i++) {
    ptr[i] = (char)x;
    x >>= 8;
  }
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

uint64_t state_read_mem_uint64(char *ptr)
{
  uint64_t value = 0;
  int i;
  for(i=0;i<8;i++) {
    value <<= 8;
    value |= ptr[i];
  }
  return value;
}

void *state_read_mem_ptr(char *ptr)
{
  intptr_t x = 0;
  int i;
  for(i = 0; i < sizeof x; i++) {
    x += (ptr[i] & 0xff) << (i*8);
  }
  return (void *)x;
}

static void state_write_file_long(FILE *f, long data)
{
  fprintf(f, "%c", (unsigned char)(data>>24));
  fprintf(f, "%c", (unsigned char)(data>>16));
  fprintf(f, "%c", (unsigned char)(data>>8));
  fprintf(f, "%c", (unsigned char)(data));
}

static long state_read_file_long(FILE *f)
{
  unsigned char tmp[4];
  if(fread(tmp, 4, 1, f) != 1)
    WARNING(fread);

  return (tmp[0]<<24)|(tmp[1]<<16)|(tmp[2]<<8)|tmp[3];
}

static void state_write_section_header(FILE *f, char *id, long size)
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

static int state_read_section_header(FILE *f, struct state_section *sec)
{
  if(fread(sec->id, 4, 1, f) != 1) {
    return STATE_INVALID;
  }
  sec->size = state_read_file_long(f);
  return STATE_VALID;
}

static int state_read_section(FILE *f, struct state_section *sec)
{
  char dummy[4];

  if(state_read_section_header(f, sec) == STATE_INVALID)
    return STATE_INVALID;

  if(sec->size == 0) {
    sec->data = NULL;
    return STATE_VALID;
  }
  
  sec->data = (char *)xmalloc(sec->size);
  if(sec->data == NULL) return STATE_INVALID;
  if(fread(sec->data, sec->size, 1, f) != 1) {
    return STATE_INVALID;
  }
  if(sec->size%4)
    if(fread(dummy, 4-(sec->size%4), 1, f) != 1)
      WARNING(fread);

  return STATE_VALID;
}

static void state_clear_mmu(struct mmu_state *state)
{
  while(state) {
    free(state->data);
    state = state->next;
  }
}

static void state_clear(struct state *state)
{
  if(state == NULL) return;

  if(state->cpu_state) free(state->cpu_state->data);
  if(state->mmu_state) state_clear_mmu(state->mmu_state);
  free(state);
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

struct cpu_state *state_cpu_state_dup(struct cpu_state *cpu_state)
{
  struct cpu_state *new;

  new = (struct cpu_state *)xmalloc(sizeof(struct cpu_state));

  strncpy(new->id, cpu_state->id, 4);
  new->size = cpu_state->size;
  new->data = (char *)xmalloc(new->size);
  memcpy(new->data, cpu_state->data, new->size);

  return new;
}

struct mmu_state *state_mmu_state_dup(struct mmu_state *mmu_state)
{
  struct mmu_state *first,*new;

  first = NULL;

  while(mmu_state) {
    new = (struct mmu_state *)xmalloc(sizeof(struct mmu_state));
    
    strncpy(new->id, mmu_state->id, 4);
    new->size = mmu_state->size;
    new->data = (char *)xmalloc(new->size);
    memcpy(new->data, mmu_state->data, new->size);
    new->next = first;
    first = new;
    mmu_state = mmu_state->next;
  }

  return first;
}

struct state *state_dup(struct state *state)
{
  struct state *new;

  new = (struct state *)xmalloc(sizeof(struct state));
  if(new == NULL) return NULL;

  new->id = state->id;
  new->version = state->version;
  new->size = state->size;
  new->timestamp = state->timestamp;
  new->delta = state->delta;
  new->cpu_state = state_cpu_state_dup(state->cpu_state);
  new->mmu_state = state_mmu_state_dup(state->mmu_state);

  return new;
}

struct state *state_collect()
{
  struct state *new;
  struct state_list *new_list;
  long before;

  before = SDL_GetTicks();

  new = (struct state *)xmalloc(sizeof(struct state));
  if(new == NULL) return NULL;

  new_list = (struct state_list *)xmalloc(sizeof(struct state_list));
  if(new_list == NULL) return NULL;
  
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

  maxid++;
  new->size = new->cpu_state->size + state_get_mmu_size(new->mmu_state);
  new->id = maxid;
  new->version = STATE_VERSION;
  new->timestamp = 0;
  new->delta = 0;

  new_list->next = list;
  new_list->state = new;
  list = new_list;

  printf("Time to save: %ld ticks\n", SDL_GetTicks()-before);

  return new;
}

static struct state *state_find_state_by_id(long id)
{
  struct state_list *t;

  t = list;

  while(t) {
    if(id == t->state->id) return t->state;
    t = t->next;
  }

  return NULL;
}

static long state_compress_rle(char *tmp, long size, char **input)
{
  char last;
  char *rle;
  int i,cnt,j;

  rle = (char *)xmalloc(size*2+8);
  *input = rle;
  if(rle == NULL) {
    *input = NULL;
    return 0;
  }

  strncpy(rle, "RLE!", 4);
  state_write_mem_long(&rle[4], size);
  last = tmp[0];
  cnt = 1;
  j = 8; /* Skip RLE! header */

  for(i=1;i<size;i++) {
    if((last == tmp[i]) && (i!=(size-1))) {
      cnt++;
      if(cnt >= 65534) {
        rle[j++] = last;
        rle[j++] = last;
        rle[j++] = cnt>>8;
        rle[j++] = cnt;
        cnt = 0;
      }
    } else {
      if(i==(size-1)) {
        if(last == tmp[i]) {
          cnt++;
        }
      }
      if(cnt > 1) {
        rle[j++] = last;
        rle[j++] = last;
        rle[j++] = cnt>>8;
        rle[j++] = cnt;
        cnt = 1;
      } else {
        rle[j++] = last;
      }
      if(i==(size-1)) {
        if(last != tmp[i]) {
          rle[j++] = tmp[i];
        }
      }
    }
    last = tmp[i];
  }

  if(j >= size) {
    free(rle);
    *input = NULL;
    return 0;
  }
  
  rle = (char *)xmalloc(j);
  memcpy(rle, *input, j);
  free(*input);
  *input = rle;

  return j;
}

struct state *state_encode_delta(struct state *state, struct state *ref)
{
  struct state *new;
  char *tmp,*rle;
  int i;
  long rle_size;

  tmp = (char *)xmalloc(state->cpu_state->size);
  if(tmp == NULL) return NULL;
  new = (struct state *)xmalloc(sizeof(struct state));
  if(new == NULL) return NULL;

  new->id = state->id;
  new->version = state->version;
  new->timestamp = state->timestamp;
  new->delta = ref->id;

  for(i=0;i<state->cpu_state->size;i++) {
    tmp[i] = state->cpu_state->data[i]^ref->cpu_state->data[i];
  }
  rle_size = state_compress_rle(tmp, state->cpu_state->size, &rle);
  new->cpu_state = state->cpu_state;
  state->cpu_state = NULL;
  free(new->cpu_state->data);
  if(rle_size == 0) {
    new->cpu_state->data = tmp;
  } else {
    new->cpu_state->size = rle_size;
    new->cpu_state->data = rle;
    free(tmp);
  }

  return new;
}

static long state_uncompress_rle(char *rle, long size, char **output)
{
  int i,j;
  long usize,ucnt;
  unsigned int cnt;

  if(strncmp("RLE!", rle, 4)) {
    *output = xmalloc(size);
    memcpy(*output, rle, size);
    return size;
  }

  usize = state_read_mem_long(&rle[4]);
  ucnt = 0;
  *output = xmalloc(usize);

  for(i=0;i<size;i++) {
    if(i < (size-4) && (rle[i] == rle[i+1])) {
      cnt = (rle[i+2]<<8)|rle[i+3];
      if((ucnt+cnt) > usize) {
	fprintf(stderr, "ERROR! Illegal RLE data\n");
	free(*output);
	*output = NULL;
	return 0;
      }
      for(j=0;j<cnt;j++) {
	*output[ucnt++] = rle[i];
      }
    } else {
      *output[ucnt++] = rle[i];
      if(ucnt > usize) {
	fprintf(stderr, "ERROR! Illegal RLE data\n");
	free(*output);
	*output = NULL;
	return 0;
      }
    }
  }
  if(ucnt != usize) {
    fprintf(stderr, "ERROR! Illegal RLE data\n");
    free(*output);
    *output = NULL;
    return 0;
  }

  return usize;
}

static struct mmu_state *state_find_mmu_state_id(struct state *state,
						 char *id)
{
  struct mmu_state *mmu;
  
  mmu = state->mmu_state;
  while(mmu) {
    if(!strncmp(mmu->id, id, 4)) return mmu;
    mmu = mmu->next;
  }
  return NULL;
}

struct state *state_decode_delta(struct state *coded, struct state *ref)
{
  struct state *state;
  struct mmu_state *mmu,*mmuref;
  char *tmp,*unrle;
  int i;
  long usize;
  
  state = state_dup(coded);
  if(state == NULL) return NULL;

  tmp = (char *)xmalloc(ref->cpu_state->size);
  if(tmp == NULL) return NULL;

  usize = state_uncompress_rle(state->cpu_state->data,
			       state->cpu_state->size,
			       &unrle);
  if(usize == 0) {
    fprintf(stderr, "ERROR! RLE uncompression failed.\n");
    return NULL;
  }

  if(usize != ref->cpu_state->size) {
    free(unrle);
    fprintf(stderr, "ERROR! State sizes conflict\n");
    return NULL;
  }

  for(i=0;i<usize;i++) {
    tmp[i] = unrle[i]^ref->cpu_state->data[i];
  }

  free(state->cpu_state->data);
  state->cpu_state->data = tmp;
  state->cpu_state->size = usize;

  free(unrle);

  mmu = state->mmu_state;

  while(mmu) {
    usize = state_uncompress_rle(mmu->data, mmu->size, &unrle);

    if(usize == 0) {
      fprintf(stderr, "ERROR! RLE uncompression failed.\n");
      return NULL;
    }

    mmuref = state_find_mmu_state_id(ref, mmu->id);
    if(mmuref == NULL) {
      fprintf(stderr, "ERROR! MMU Reference data missing.\n");
      return NULL;
    }
    
    if(usize != mmuref->size) {
      free(unrle);
      fprintf(stderr, "ERROR! State sizes conflict\n");
      return NULL;
    }
    
    tmp = (char *)xmalloc(usize);
    if(tmp == NULL) return NULL;
    
    for(i=0;i<usize;i++) {
      tmp[i] = unrle[i]^mmuref->data[i];
    }
    
    free(mmu->data);
    mmu->data = tmp;
    mmu->size = usize;
    
    free(unrle);

    mmu = mmu->next;
  }
  
  return state;
}

struct state *state_unpack_delta(struct state *state)
{
  struct state *tmp, *result;

  if(state == NULL) return NULL;
  return NULL;

  if(state->delta) {
    tmp = state_unpack_delta(state_find_state_by_id(state->delta));
    if(tmp == NULL) return NULL;
    result = state_decode_delta(state, tmp);
    if(result == NULL) {
      if(tmp->delta == -1)
	state_clear(tmp);
      return NULL;
    }
    result->delta = -1;
    if(tmp->delta == -1)
      state_clear(tmp);
    return result;
  }
  
  return state;
}

struct state *state_pack_delta(struct state *state, struct state *ref)
{
  struct state *tmp, *new;
  struct state_list *new_list;

  tmp = state_unpack_delta(ref);
  new = state_encode_delta(state, tmp);
  if(tmp->delta == -1)
    state_clear(tmp);
  state_remove(state);
  new_list = (struct state_list *)xmalloc(sizeof(struct state_list));
  if(new_list == NULL) {
    state_clear(new);
    return NULL;
  }
  new_list->state = new;
  new_list->next = list;
  list = new_list;
  
  return new;
}

void state_restore(struct state *state)
{
  if(state->version != STATE_VERSION) {
    fprintf(stderr, "Incompatible state version %ld != %d\n",
	    state->version, STATE_VERSION);
    return;
  }
  cpu_state_restore(state->cpu_state);
  mmu_state_restore(state->mmu_state);
}

void state_remove(struct state *state)
{
  struct state_list *t,*tmp;

  t = list;
  if(t == NULL) return;
  if(t->state == state) {
    state_clear(t->state);
    list = t->next;
    free(t);
    return;
  }
  
  while(t) {
    if(t->next != NULL) {
      if(t->next->state == state) {
	state_clear(state);
	tmp = t->next->next;
	free(t->next);
	t->next = tmp;
	return;
      }
    }
    t = t->next;
  }
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
  FILE *f;
  struct state *new;
  struct state_section sec;
  struct mmu_state *t;
  
  new = (struct state *)xmalloc(sizeof(struct state));
  if(new == NULL) return NULL;
  new->mmu_state = NULL;

  f = fopen(filename, "rb");
  state_read_section_header(f, &sec);
  if(strncmp(sec.id, "STAT", 4)) {
    free(new);
    return NULL;
  }

  while(state_read_section(f, &sec)) {
    if(!strncmp(sec.id, "STID", 4)) {
      if(sec.size != 4) {
	free(sec.data);
	free(new);
	return NULL;
      }
      new->id = state_read_mem_long(sec.data);
      free(sec.data);
    } else if(!strncmp(sec.id, "STVE", 4)) {
      if(sec.size != 4) {
	free(sec.data);
	free(new);
	return NULL;
      }
      new->version = state_read_mem_long(sec.data);
      free(sec.data);
    } else if(!strncmp(sec.id, "STTS", 4)) {
      if(sec.size != 8) {
	free(sec.data);
	free(new);
	return NULL;
      }
      free(sec.data);
    } else if(!strncmp(sec.id, "STDL", 4)) {
      if(sec.size != 4) {
	free(sec.data);
	free(new);
	return NULL;
      }
      free(sec.data);
    } else if(!strncmp(sec.id, "CPU0", 4)) {
      new->cpu_state = (struct cpu_state *)xmalloc(sizeof(struct cpu_state));
      if(new->cpu_state == NULL) {
	free(new);
	free(sec.data);
	return NULL;
      }
      new->cpu_state->data = (char *)xmalloc(sec.size);
      if(new->cpu_state->data == NULL) {
	free(new->cpu_state);
	free(new);
	free(sec.data);
	return NULL;
      }
      strncpy(new->cpu_state->id, sec.id, 4);
      new->cpu_state->size = sec.size;
      memcpy(new->cpu_state->data, sec.data, sec.size);
      free(sec.data);
    } else {
      t = (struct mmu_state *)xmalloc(sizeof(struct mmu_state));
      if(t == NULL) {
	free(new);
	free(sec.data);
	return NULL;
      }
      t->data = (char *)xmalloc(sec.size);
      if(t->data == NULL) {
	free(t);
	free(new);
	free(sec.data);
	return NULL;
      }
      strncpy(t->id, sec.id, 4);
      t->size = sec.size;
      memcpy(t->data, sec.data, sec.size);
      free(sec.data);
      t->next = new->mmu_state;
      new->mmu_state = t;
    }
  }
  
  return new;
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

