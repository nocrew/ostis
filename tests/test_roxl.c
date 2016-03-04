#include "test_main.h"
#include "cpu.h"
#include "cprint.h"

#define TEST_HOOK_TEST1 1

static int print_test_code_on_next_before = 0;
static int print_test_code = 0;

static void test_roxl_hook_init(struct cpu *cpu)
{
  print_test_code_on_next_before = 1;
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
  print_test_code = 0;
  if(!CHKX) {
    printf("Test2 failed. Expected X to be set, but it was not\n");
    exit(-1);
  }
  printf("Test2 successful.\n");
  exit(0);
}

static void test_roxl_hook_before_instr(struct cpu *cpu)
{
  if(print_test_code_on_next_before) {
    print_test_code_on_next_before = 0;
    print_test_code = 1;
  }
  struct cprint *cprint;
  if(print_test_code) {
    cprint = cprint_instr(cpu->pc);
    printf("Test-BEFORE: PC == %06x, SR == %04x    %s %s\n", cpu->pc, cpu->sr, cprint->instr, cprint->data);
  }
}

static void test_roxl_hook_after_instr(struct cpu *cpu)
{
  if(print_test_code) {
    printf("Test-AFTER:  PC == %06x, SR == %04x\n", cpu->pc, cpu->sr);
  }
}

void test_roxl_init()
{
  struct test_case *test_case;
  
  test_case = test_case_alloc("roxl");

  test_case->hooks[TEST_HOOK_INIT] = test_roxl_hook_init;
  test_case->hooks[TEST_HOOK_TEST1] = test_roxl_hook_test1;
  test_case->hooks[TEST_HOOK_EXIT] = test_roxl_hook_exit;
  test_case->hooks[TEST_HOOK_BEFORE_INSTR] = test_roxl_hook_before_instr;
  test_case->hooks[TEST_HOOK_AFTER_INSTR] = test_roxl_hook_after_instr;
  
  test_case_register(test_case);
}
