#include "test_main.h"
#include "cpu.h"

static void test_moveq_hook_init(struct cpu *cpu)
{
  cpu->d[0] = 0;
}

static void test_moveq_hook_exit(struct cpu *cpu)
{
  if(cpu->d[0] != 98) {
    printf("Test failed. Expected 98, Got %d\n", cpu->d[0]);
    exit(-1);
  }
  printf("Test successful.\n");
  exit(0);
}

void test_moveq_init()
{
  struct test_case *test_case;
  
  test_case = test_case_alloc("moveq");

  test_case->hooks[TEST_HOOK_INIT] = test_moveq_hook_init;
  test_case->hooks[TEST_HOOK_EXIT] = test_moveq_hook_exit;
  
  test_case_register(test_case);
}
