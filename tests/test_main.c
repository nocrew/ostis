#include <sys/stat.h>
#include "common.h"
#include "test_main.h"
#include "test_cases.h"
#include "cpu.h"
#include "mmu.h"

#define TEST_DATA_DIR "tests/data"
#define TEST_PREFIX "test_"
#define TEST_SUFFIX_CART ".cart"
#define TEST_SUFFIX_FLOPPY_STLOWER ".st"
#define TEST_SUFFIX_FLOPPY_STUPPER ".ST"
#define TEST_SUFFIX_FLOPPY_MSALOWER ".msa"
#define TEST_SUFFIX_FLOPPY_MSAUPPER ".MSA"

static struct test_case_item *items = NULL;
static struct test_case *current_case = NULL;

static void test_case_default_init(struct cpu *cpu)
{
  printf("This is the default init function\n");
}

static void test_case_default_exit(struct cpu *cpu)
{
  printf("This is the default exit function\n");
  exit(-1);
}

static void test_cpu_bkpt(struct cpu *cpu, WORD op)
{
  int bkpt = op&0x7;

  if(current_case && current_case->hooks[bkpt]) {
    current_case->hooks[bkpt](cpu);
  }
}

static struct cprint *test_cpu_bkpt_print(LONG addr, WORD op)
{
  struct cprint *ret;

  ret = cprint_alloc(addr);
  strcpy(ret->instr, "BKPT");
  sprintf(ret->data, "#%d", op&0x7);
  
  return ret;
}

void test_cpu_init(void *instr[], void *print[])
{
  int i;
  for(i=0;i<8;i++) {
    instr[0x4848|i] = test_cpu_bkpt;
    print[0x4848|i] = test_cpu_bkpt_print;
  }
}

void test_call_hooks(int hooknum, struct cpu *cpu)
{
  if(current_case && current_case->hooks[hooknum]) {
    current_case->hooks[hooknum](cpu);
  }
}

static void find_data_file(char *case_name, struct test_case *test_case)
{
  struct stat buf;
  char *filename;
  /* This allocates the longest case of filenames to check */
  filename = (char *)malloc(strlen(TEST_DATA_DIR) + strlen(TEST_DATA_DIR) + strlen(case_name) + strlen(TEST_SUFFIX_CART) + 1);

  /* Check for cartridge */
  sprintf(filename, "%s/%s%s%s", TEST_DATA_DIR, TEST_PREFIX, case_name, TEST_SUFFIX_CART);
  if(stat(filename, &buf) == 0) {
    test_case->cartridge_name = strdup(filename);
    return;
  }

  /* Check for floppy (.st) */
  sprintf(filename, "%s/%s%s%s", TEST_DATA_DIR, TEST_PREFIX, case_name, TEST_SUFFIX_FLOPPY_STLOWER);
  if(stat(filename, &buf) == 0) {
    test_case->floppy_name = strdup(filename);
    return;
  }

  /* Check for floppy (.ST) */
  sprintf(filename, "%s/%s%s%s", TEST_DATA_DIR, TEST_PREFIX, case_name, TEST_SUFFIX_FLOPPY_STUPPER);
  if(stat(filename, &buf) == 0) {
    test_case->floppy_name = strdup(filename);
    return;
  }

  /* Check for floppy (.msa) */
  sprintf(filename, "%s/%s%s%s", TEST_DATA_DIR, TEST_PREFIX, case_name, TEST_SUFFIX_FLOPPY_MSALOWER);
  if(stat(filename, &buf) == 0) {
    test_case->floppy_name = strdup(filename);
    return;
  }

  /* Check for floppy (.MSA) */
  sprintf(filename, "%s/%s%s%s", TEST_DATA_DIR, TEST_PREFIX, case_name, TEST_SUFFIX_FLOPPY_MSAUPPER);
  if(stat(filename, &buf) == 0) {
    test_case->floppy_name = strdup(filename);
    return;
  }
}

struct test_case *test_case_alloc(char *case_name)
{
  int i;
  struct test_case *test_case = (struct test_case *)malloc(sizeof(struct test_case));
  
  for(i=0;i<TEST_HOOK_MAXCODE;i++) {
    test_case->hooks[i] = NULL;
  }
  
  test_case->case_name = strdup(case_name);
  test_case->cartridge_name = NULL;
  test_case->floppy_name = NULL;
  
  find_data_file(case_name, test_case);

  test_case->hooks[TEST_HOOK_INIT] = test_case_default_init;
  test_case->hooks[TEST_HOOK_EXIT] = test_case_default_exit;

  return test_case;
}

static struct test_case *find_case(char *case_name)
{
  struct test_case_item *t;

  t = items;

  while(t) {
    if(!strcmp(case_name, t->test_case->case_name))
      return t->test_case;
    t = t->next;
  }

  return NULL;
}

void test_case_register(struct test_case *test_case)
{
  struct test_case_item *new;

  if(!test_case) return;
  
  if(find_case(test_case->case_name) == NULL) {
    new = (struct test_case_item *)malloc(sizeof(struct test_case_item));
    if(new == NULL) {
      fprintf(stderr, "FATAL: Could not allocate test case item struct\n");
      exit(-1);
    }
    new->test_case = test_case;
    new->next = items;
    items = new;
  }
}

struct test_case *test_init(char *case_name)
{
  test_moveq_init();
  test_roxl_init();
  test_roxr_init();
  test_lsl_init();
  current_case = find_case(case_name);

  return current_case;
}

