#include <stdio.h>
#include <string.h>
#include "common.h"
#include "cpu.h"
#include "mfp.h"
#include "shifter.h"
#include "ikbd.h"
#include "psg.h"
#include "fdc.h"
#include "mmu.h"
#include "state.h"

/* Used to prevent extra cycle counts when reading from memory for the only purpose of printing the value */
int mmu_print_state = 0;

static uint8_t mmu_module_at_addr[1<<24];
static uint8_t mmu_module_count = 0;
static struct mmu *mmu_module_by_id[256];
static struct mmu_module *modules = NULL;
static uint8_t bus_error_id;
static uint8_t bus_error_odd_addr_id;

static struct mmu_module *find_module(struct mmu *data)
{
  struct mmu_module *t;

  t = modules;

  while(t) {
    if((t->module->start == data->start) &&
       (t->module->size == data->size))
      return t;
    t = t->next;
  }

  return NULL;
}

static struct mmu_module *find_module_by_id(char *id)
{
  struct mmu_module *t;

  t = modules;

  while(t) {
    if(!strncmp(id, t->module->id, 4))
      return t;
    t = t->next;
  }

  return NULL;
}

void mmu_send_bus_error(int reading, LONG addr)
{
  int flags = 0;

  if(reading) {
    flags |= CPU_BUSERR_READ;
  } else {
    flags |= CPU_BUSERR_WRITE;
  }
  if(cpu->pc == addr) {
    flags |= CPU_BUSERR_INSTR;
  } else {
    flags |= CPU_BUSERR_DATA;
  }
  
  fprintf(stdout, "BUS ERROR at 0x%08x\n", addr);
  cpu_print_status(CPU_USE_LAST_PC);
  
  cpu_set_bus_error(flags, addr);
}

void mmu_send_address_error(int reading, LONG addr)
{
  int flags = 0;

  if(reading) {
    flags |= CPU_ADDRERR_READ;
  } else {
    flags |= CPU_ADDRERR_WRITE;
  }
  if(cpu->pc == addr) {
    flags |= CPU_ADDRERR_INSTR;
  } else {
    flags |= CPU_ADDRERR_DATA;
  }
  
  fprintf(stdout, "ADDRESS ERROR at 0x%08x\n", addr);
  cpu_print_status(CPU_USE_LAST_PC);
  
  cpu_set_address_error(flags, addr);
}

static BYTE mmu_read_byte_bus_error(LONG addr)
{
  mmu_send_bus_error(CPU_BUSERR_READ, addr);
  return 0;
}

static WORD mmu_read_word_bus_error(LONG addr)
{
  mmu_send_bus_error(CPU_BUSERR_READ, addr);
  return 0;
}

static LONG mmu_read_long_bus_error(LONG addr)
{
  mmu_send_bus_error(CPU_BUSERR_READ, addr);
  return 0;
}

static void mmu_write_byte_bus_error(LONG addr, BYTE data)
{
  mmu_send_bus_error(CPU_BUSERR_WRITE, addr);
}

static void mmu_write_word_bus_error(LONG addr, WORD data)
{
  mmu_send_bus_error(CPU_BUSERR_WRITE, addr);
}

static void mmu_write_long_bus_error(LONG addr, LONG data)
{
  mmu_send_bus_error(CPU_BUSERR_WRITE, addr);
}

static WORD mmu_read_word_address_error(LONG addr)
{
  mmu_send_address_error(CPU_ADDRERR_READ, addr);
  return 0;
}

static LONG mmu_read_long_address_error(LONG addr)
{
  mmu_send_address_error(CPU_ADDRERR_READ, addr);
  return 0;
}

static void mmu_write_word_address_error(LONG addr, WORD data)
{
  mmu_send_address_error(CPU_ADDRERR_WRITE, addr);
}

static void mmu_write_long_address_error(LONG addr, LONG data)
{
  mmu_send_address_error(CPU_ADDRERR_WRITE, addr);
}

static struct mmu *mmu_bus_error_module()
{
  struct mmu *bus_error;

  bus_error = (struct mmu *)malloc(sizeof(struct mmu));

  memcpy(bus_error->id, "BSER", 4);
  bus_error->name = strdup("BSER");
  bus_error->read_byte = mmu_read_byte_bus_error;
  bus_error->read_word = mmu_read_word_bus_error;
  bus_error->read_long = mmu_read_long_bus_error;
  bus_error->write_byte = mmu_write_byte_bus_error;
  bus_error->write_word = mmu_write_word_bus_error;
  bus_error->write_long = mmu_write_long_bus_error;

  return bus_error;
}

static struct mmu *mmu_clone_module(struct mmu *module)
{
  struct mmu *clone;
  clone = (struct mmu *)malloc(sizeof(struct mmu));

  memcpy(clone->id, "BSER", 4);
  clone->name = strdup("BSER");
  clone->read_byte = module->read_byte;
  clone->read_word =  module->read_word;
  clone->read_long =  module->read_long;
  clone->write_byte = module->write_byte;
  clone->write_word = module->write_word;
  clone->write_long = module->write_long;

  return clone;
}

static struct mmu *mmu_clone_module_for_address_error(struct mmu *module)
{
  struct mmu *address_error_clone;
  address_error_clone= mmu_clone_module(module);
  address_error_clone->read_word = mmu_read_word_address_error;
  address_error_clone->read_long = mmu_read_long_address_error;
  address_error_clone->write_word = mmu_write_word_address_error;
  address_error_clone->write_long = mmu_write_long_address_error;

  return address_error_clone;
}

static uint8_t mmu_get_module_id(struct mmu *module)
{
  uint8_t id;
  id = mmu_module_count;
  mmu_module_by_id[id] = module;
  mmu_module_count++;
  return id;
}

void mmu_init()
{
  int i;
  struct mmu *bus_error_module;
  struct mmu *bus_error_module_odd_addr;

  bus_error_module = mmu_bus_error_module();
  bus_error_id = mmu_get_module_id(bus_error_module);

  bus_error_module_odd_addr = mmu_clone_module_for_address_error(bus_error_module);
  bus_error_odd_addr_id = mmu_get_module_id(bus_error_module_odd_addr);
  
  /* Populate entire memory space with default module that gives bus error or address error */
  for(i=0;i<16777216;i++) {
    if(i&1) {
      /* Odd addresses will give address errors on WORD/LONG */
      mmu_module_at_addr[i] = bus_error_odd_addr_id;
    } else {
      mmu_module_at_addr[i] = bus_error_id;
    }
  }
}

BYTE mmu_read_byte_print(LONG addr)
{
  BYTE value;
  addr &= 0xffffff;
  if(mmu_module_at_addr[addr] == bus_error_id || mmu_module_at_addr[addr] == bus_error_odd_addr_id) {
    return 0;
  }
  mmu_print_state = 1;
  value = mmu_module_by_id[mmu_module_at_addr[addr]]->read_byte(addr);
  mmu_print_state = 0;
  return value;
}

WORD mmu_read_word_print(LONG addr)
{
  BYTE low,high;
  addr &= 0xffffff;

  high = mmu_read_byte_print(addr);
  low = mmu_read_byte_print(addr+1);

  return (high<<8)|low;
}

LONG mmu_read_long_print(LONG addr)
{
  WORD low,high;
  addr &= 0xffffff;

  high = mmu_read_word_print(addr);
  low = mmu_read_word_print(addr+2);

  return (high<<16)|low;
}

BYTE mmu_read_byte(LONG addr)
{
  addr &= 0xffffff;
  return mmu_module_by_id[mmu_module_at_addr[addr]]->read_byte(addr);
}

WORD mmu_read_word(LONG addr)
{
  addr &= 0xffffff;
  return mmu_module_by_id[mmu_module_at_addr[addr]]->read_word(addr);
}

LONG mmu_read_long(LONG addr)
{
  addr &= 0xffffff;
  return mmu_module_by_id[mmu_module_at_addr[addr]]->read_long(addr);
}

void mmu_write_byte(LONG addr, BYTE data)
{
  addr &= 0xffffff;
  mmu_module_by_id[mmu_module_at_addr[addr]]->write_byte(addr, data);
}

void mmu_write_word(LONG addr, WORD data)
{
  addr &= 0xffffff;
  mmu_module_by_id[mmu_module_at_addr[addr]]->write_word(addr, data);
}

void mmu_write_long(LONG addr, LONG data)
{
  addr &= 0xffffff;
  mmu_module_by_id[mmu_module_at_addr[addr]]->write_long(addr, data);
}

struct mmu_state *mmu_state_collect()
{
  struct mmu_module *t;
  struct mmu_state *new,*top;

  top = NULL;

  t = modules;

  while(t) {
    if(t->module->state_collect != NULL) {
      new = (struct mmu_state *)malloc(sizeof(struct mmu_state));
      if(new != NULL) {
	strncpy(new->id, t->module->id, 4);
	if(t->module->state_collect(new) == STATE_VALID) {
	  new->next = top;
	  top = new;
	} else {
	  free(new);
	}
      }
    }
    t = t->next;
  }
  
  return top;
}

void mmu_state_restore(struct mmu_state *state)
{
  struct mmu_module *module;
  struct mmu_state *t;

  t = state;
  
  while(t) {
    module = find_module_by_id(t->id);
    if(module != NULL) {
      module->module->state_restore(t);
    }
    t = t->next;
  }
}

void mmu_register(struct mmu *data)
{
  struct mmu_module *new;
  struct mmu *data_address_error;
  uint8_t data_id, data_address_error_id;
  int i,addr;

  if(!data) return;
  
  if(find_module(data) == NULL) {
    new = (struct mmu_module *)malloc(sizeof(struct mmu_module));
    if(new == NULL) {
      fprintf(stderr, "FATAL: Could not allocate module struct\n");
      exit(-1);
    }
    new->module = data;
    new->next = modules;
    modules = new;
  }

  data_id = mmu_get_module_id(data);
  data_address_error = mmu_clone_module_for_address_error(data);
  data_address_error_id = mmu_get_module_id(data_address_error);

  for(i=0;i<data->size;i++) {
    addr = data->start + i;
    if(addr&1) {
      mmu_module_at_addr[addr] = data_address_error_id;
    } else {
      mmu_module_at_addr[addr] = data_id;
    }
  }
}

void mmu_print_map()
{
  struct mmu_module *t;

  t = modules;
  
  while(t) {
    printf("Name : %s\n", t->module->name);
    printf("Start: 0x%08x\n", t->module->start);
    printf("End  : 0x%08x\n", t->module->start+t->module->size-1);
    printf("Size : %d\n", t->module->size);
    printf("\n");
    t = t->next;
  }
}

void mmu_do_interrupts(struct cpu *cpu)
{
  mfp_do_interrupts(cpu);
  fdc_do_interrupts(cpu);
  ikbd_do_interrupts(cpu);
  psg_do_interrupts(cpu);
}

