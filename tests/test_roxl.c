#include "test_main.h"
#include "cpu.h"

#define TEST_HOOK_TEST1 1

static void test_roxl_hook_init(struct cpu *cpu)
{
}

static void test_roxl_hook_test1(struct cpu *cpu)
{
  if(!CHKX) {
    printf("Test1 failed. Expected X to be set, but it was not\n");
    exit(-1);
  }
  if(cpu->d[2] != 2) {
    printf("Test1 failed. Expected 2, Got %d\n", cpu->d[2]);
    exit(-1);
  }
  printf("Test1 successful.\n");
}

static void test_roxl_hook_exit(struct cpu *cpu)
{
  if(!CHKX) {
    printf("Test1 failed. Expected X to be set, but it was not\n");
    exit(-1);
  }
  printf("Test2 successful.\n");
  exit(0);
}

void test_roxl_init()
{
  struct test_case *test_case;
  
  test_case = test_case_alloc("roxl");

  test_case->hooks[TEST_HOOK_INIT] = test_roxl_hook_init;
  test_case->hooks[TEST_HOOK_TEST1] = test_roxl_hook_test1;
  test_case->hooks[TEST_HOOK_EXIT] = test_roxl_hook_exit;
  
  test_case_register(test_case);
}
