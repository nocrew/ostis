#include "test_main.h"
#include "cpu.h"

#define TEST_HOOK_TEST1 1

static void test_lsl_hook_init(struct cpu *cpu)
{
}

static void test_lsl_hook_test1(struct cpu *cpu)
{
  if(!CHKX) {
    printf("Test1 failed. Expected X to be set, but it was not\n");
    exit(-1);
  }
  if(!CHKC) {
    printf("Test1 failed. Expected C to be set, but it was not\n");
    exit(-1);
  }
  printf("Test1 successful.\n");
}

static void test_lsl_hook_exit(struct cpu *cpu)
{
  if(CHKX) {
    printf("Test2 failed. Expected X to be unset, but it was set\n");
    exit(-1);
  }
  if(CHKC) {
    printf("Test2 failed. Expected C to be unset, but it was set\n");
    exit(-1);
  }
  printf("Test2 successful.\n");
  exit(0);
}

void test_lsl_init()
{
  struct test_case *test_case;
  
  test_case = test_case_alloc("lsl");

  test_case->hooks[TEST_HOOK_INIT] = test_lsl_hook_init;
  test_case->hooks[TEST_HOOK_TEST1] = test_lsl_hook_test1;
  test_case->hooks[TEST_HOOK_EXIT] = test_lsl_hook_exit;
  
  test_case_register(test_case);
}
