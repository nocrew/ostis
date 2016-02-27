#ifndef TEST_BUILD_H
#define TEST_BUILD_H

#include "cpu.h"

#define TEST_OK 0
#define TEST_HOOK_INIT    0
#define TEST_HOOK_EXIT    7
#define TEST_HOOK_MAXCODE 8

typedef void (*test_case_function)(struct cpu *);

struct test_case {
  char *case_name;
  char *cartridge_name;
  char *floppy_name;
  test_case_function hooks[TEST_HOOK_MAXCODE];
};

struct test_case_item {
  struct test_case_item *next;
  struct test_case *test_case;
};

struct test_case *test_init(char *);
struct test_case *test_case_alloc();
void test_cpu_init(void *[], void *[]);
void test_call_hooks(int, struct cpu *);
void test_case_register(struct test_case *);

#endif
