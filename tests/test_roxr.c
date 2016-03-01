#include "test_main.h"
#include "cpu.h"

static void test_roxr_hook_init(struct cpu *cpu)
{
}

static void test_roxr_hook_exit(struct cpu *cpu)
{
  if(cpu->d[0] != 0x00008000) {
    printf("Test failed. Expected D0 to be $00008000, but got $%08x\n", cpu->d[0]);
    exit(-1);
  }
  printf("Test successful.\n");
  exit(0);
}

void test_roxr_init()
{
  struct test_case *test_case;
  
  test_case = test_case_alloc("roxr");

  test_case->hooks[TEST_HOOK_INIT] = test_roxr_hook_init;
  test_case->hooks[TEST_HOOK_EXIT] = test_roxr_hook_exit;
  
  test_case_register(test_case);
}
