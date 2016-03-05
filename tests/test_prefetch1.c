#include "test_main.h"
#include "cpu.h"

static void test_prefetch1_hook_init(struct cpu *cpu)
{
}

static void test_prefetch1_hook_exit(struct cpu *cpu)
{
  if(cpu->d[0] != 1) {
    printf("Test failed. Expected 1, Got %d\n", cpu->d[0]);
    exit(-1);
  }
  printf("Test successful.\n");
  exit(0);
}

void test_prefetch1_init()
{
  struct test_case *test_case;
  
  test_case = test_case_alloc("prefetch1");

  test_case->hooks[TEST_HOOK_INIT] = test_prefetch1_hook_init;
  test_case->hooks[TEST_HOOK_EXIT] = test_prefetch1_hook_exit;
  
  test_case_register(test_case);
}
