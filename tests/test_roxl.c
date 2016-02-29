#include "test_main.h"
#include "cpu.h"

static void test_roxl_hook_init(struct cpu *cpu)
{
}

static void test_roxl_hook_exit(struct cpu *cpu)
{
  if(cpu->d[2] != 2) {
    printf("Test failed. Expected 2, Got %d\n", cpu->d[2]);
    exit(-1);
  }
  printf("Test successful.\n");
  exit(0);
}

void test_roxl_init()
{
  struct test_case *test_case;
  
  test_case = test_case_alloc("roxl");

  test_case->hooks[TEST_HOOK_INIT] = test_roxl_hook_init;
  test_case->hooks[TEST_HOOK_EXIT] = test_roxl_hook_exit;
  
  test_case_register(test_case);
}
