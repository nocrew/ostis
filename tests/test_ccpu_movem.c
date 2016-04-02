#include "test_main.h"
#include "test_helpers.h"
#include "mmu.h"
#include "cpu.h"

#define TEST_HOOK_VERIFY_TEST 1
#define TEST_HOOK_SETUP_TEST  6

static uint64_t tmp_cycles;
static int test_num = 0;

static void test_ccpu_movem_hook_init(struct cpu *cpu)
{
  TEST_CASE_START("MOVEM", "Clocked CPU - MOVEM instruction");
}

static void test_ccpu_movem_setup_test1(struct cpu *cpu)
{
  cpu->d[0] = 0xffffff80;
  cpu->d[1] = 0x7f;
  cpu->d[2] = 0x55aaaa55;
  cpu->d[3] = 0xff0000ff;
  cpu->a[0] = 0x10000;
}

static void test_ccpu_movem_setup_test2(struct cpu *cpu)
{
  cpu->d[0] = 0xffffff80;
  cpu->d[1] = 0x7f;
  cpu->d[2] = 0x55aaaa55;
  cpu->d[3] = 0xff0000ff;
  cpu->a[0] = 0x10010;
}

static void test_ccpu_movem_setup_test3(struct cpu *cpu)
{
  cpu->d[0] = 0x5555aaaa;
  cpu->d[1] = 0xbbbb4444;
  cpu->d[2] = 0x77778888;
  cpu->d[3] = 0x55aa55aa;
  cpu->d[4] = 0xff00ff00;
  cpu->d[5] = 0xaa55ddee;
  cpu->d[6] = 0x3333cccc;
  cpu->d[7] = 0xabcdef42;
  cpu->a[0] = 0x20000;
}

static void test_ccpu_movem_setup_test4(struct cpu *cpu)
{
  cpu->d[0] = 0x15;
  cpu->d[1] = 0x6;
  cpu->d[2] = 0xbc;
  cpu->d[3] = 0x6;
  cpu->a[7] = 0xa048;
}

static void test_ccpu_movem_setup_test5(struct cpu *cpu)
{
  cpu->d[0] = 0x0;
  cpu->d[1] = 0x0;
  cpu->d[2] = 0x0;
  cpu->d[3] = 0x0;
}

static void test_ccpu_movem_setup_test6(struct cpu *cpu)
{
  cpu->a[0] = 0x10000;
}

static void test_ccpu_movem_setup_test7(struct cpu *cpu)
{
  int n = 0;
  
  bus_write_long(0x15de + n*4,0x80000472); n++;
  bus_write_long(0x15de + n*4,0x00000000); n++;
  bus_write_long(0x15de + n*4,0x00000000); n++;
  bus_write_long(0x15de + n*4,0x00000000); n++;
  bus_write_long(0x15de + n*4,0x00000000); n++;
  bus_write_long(0x15de + n*4,0x00000000); n++;
  bus_write_long(0x15de + n*4,0x00000000); n++;
  bus_write_long(0x15de + n*4,0x00000001); n++;
  bus_write_long(0x15de + n*4,0x000001ac); n++;
  bus_write_long(0x15de + n*4,0x0000090c); n++;
  bus_write_long(0x15de + n*4,0x00000000); n++;
  bus_write_long(0x15de + n*4,0x00000000); n++;
  bus_write_long(0x15de + n*4,0x00fc0cf8); n++;
  bus_write_long(0x15de + n*4,0x00000000); n++;
  bus_write_long(0x15de + n*4,0x000015f2); n++;
  bus_write_long(0x15de + n*4,0x000015de); n++;
  cpu->a[7] = 0x15de;
}


static void test_ccpu_movem_verify_test1(struct cpu *cpu)
{
  int i;
  LONG v1,v2,v3,v4;
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;
  
  v1 = bus_read_long_print(0x10000);
  v2 = bus_read_long_print(0x10004);
  v3 = bus_read_long_print(0x10008);
  v4 = bus_read_long_print(0x1000c);

  TEST_START(1, "MOVEM.L D0-D3,(A0)");
  EXPECT_EQ(LONG, v1, 0xffffff80, "[0x10000]");
  EXPECT_EQ(LONG, v2, 0x0000007f, "[0x10004]");
  EXPECT_EQ(LONG, v3, 0x55aaaa55, "[0x10008]");
  EXPECT_EQ(LONG, v4, 0xff0000ff, "[0x1000c]");
  EXPECT_EQ(ADDR, cpu->a[0], 0x10000, "A0");
  EXPECT_EQ(INT64, cycles, 40L, "cycle count");
  TEST_END_OK;

  for(i=0;i<8;i++) {
    bus_write_word(0x10000+i*2, 0);
  }
}

static void test_ccpu_movem_verify_test2(struct cpu *cpu)
{
  LONG v1,v2,v3,v4;
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  v1 = bus_read_long_print(0x10000);
  v2 = bus_read_long_print(0x10004);
  v3 = bus_read_long_print(0x10008);
  v4 = bus_read_long_print(0x1000c);

  TEST_START(2, "MOVEM.L D0-D3,-(A0)");
  EXPECT_EQ(LONG, v1, 0xffffff80, "[0x10000]");
  EXPECT_EQ(LONG, v2, 0x0000007f, "[0x10004]");
  EXPECT_EQ(LONG, v3, 0x55aaaa55, "[0x10008]");
  EXPECT_EQ(LONG, v4, 0xff0000ff, "[0x1000c]");
  EXPECT_EQ(ADDR, cpu->a[0], 0x10000, "A0");
  EXPECT_EQ(INT64, cycles, 40L, "cycle count");
  TEST_END_OK;

  cpu->d[4] = 0x55559999;
  cpu->d[5] = 0xaaaa9999;
  cpu->d[6] = 0x9999aaaa;
  cpu->d[7] = 0x99995555;
}

static void test_ccpu_movem_verify_test3(struct cpu *cpu)
{
  LONG v1,v2,v3,v4,v5,v6,v7,v8;
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  v1 = bus_read_long_print(0x20004);
  v2 = bus_read_long_print(0x20008);
  v3 = bus_read_long_print(0x2000c);
  v4 = bus_read_long_print(0x20010);
  v5 = bus_read_long_print(0x20014);
  v6 = bus_read_long_print(0x20018);
  v7 = bus_read_long_print(0x2001c);
  v8 = bus_read_long_print(0x20020);

  TEST_START(3, "MOVEM.L D0-D7,4(A0)");
  EXPECT_EQ(LONG, v1, 0x5555aaaa, "[0x20004]");
  EXPECT_EQ(LONG, v2, 0xbbbb4444, "[0x20008]");
  EXPECT_EQ(LONG, v3, 0x77778888, "[0x2000c]");
  EXPECT_EQ(LONG, v4, 0x55aa55aa, "[0x20010]");
  EXPECT_EQ(LONG, v5, 0xff00ff00, "[0x20014]");
  EXPECT_EQ(LONG, v6, 0xaa55ddee, "[0x20018]");
  EXPECT_EQ(LONG, v7, 0x3333cccc, "[0x2001c]");
  EXPECT_EQ(LONG, v8, 0xabcdef42, "[0x20020]");
  EXPECT_EQ(ADDR, cpu->a[0], 0x20000, "A0");
  EXPECT_EQ(INT64, cycles, 76L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_movem_verify_test4(struct cpu *cpu)
{
  WORD v1,v2,v3,v4;
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  v1 = bus_read_word_print(0xa040);
  v2 = bus_read_word_print(0xa042);
  v3 = bus_read_word_print(0xa044);
  v4 = bus_read_word_print(0xa046);

  TEST_START(4, "MOVEM.W D0-D3,-(A7)");
  EXPECT_EQ(WORD, v1, 0x0015, "[0xa040]");
  EXPECT_EQ(WORD, v2, 0x0006, "[0xa042]");
  EXPECT_EQ(WORD, v3, 0x00bc, "[0xa044]");
  EXPECT_EQ(WORD, v4, 0x0006, "[0xa046]");
  EXPECT_EQ(ADDR, cpu->a[7], 0xa040, "A7");
  EXPECT_EQ(INT64, cycles, 24L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_movem_verify_test5(struct cpu *cpu)
{
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  TEST_START(5, "MOVEM.W xxx(PC),D0-D3");
  EXPECT_EQ(LONG, cpu->d[0], 0x00001234, "D0");
  EXPECT_EQ(LONG, cpu->d[1], 0x000055aa, "D1");
  EXPECT_EQ(LONG, cpu->d[2], 0xffffff00, "D2");
  EXPECT_EQ(LONG, cpu->d[3], 0xffffaa55, "D3");
  EXPECT_EQ(INT64, cycles, 32L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_movem_verify_test6(struct cpu *cpu)
{
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  TEST_START(6, "MOVEM.W (A0)+,D4-D7");
  EXPECT_EQ(LONG, cpu->d[4], 0xffffffff, "D0");
  EXPECT_EQ(LONG, cpu->d[5], 0xffffff80, "D1");
  EXPECT_EQ(LONG, cpu->d[6], 0x00000000, "D2");
  EXPECT_EQ(LONG, cpu->d[7], 0x0000007f, "D3");
  EXPECT_EQ(INT64, cycles, 28L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_movem_verify_test7(struct cpu *cpu)
{
  TEST_START(7, "MOVEM.L (A7)+,D0-D7/A0-A7");
  EXPECT_EQ(LONG, cpu->a[7], 0x000015de, "A7");
  TEST_END_OK;
}

static void test_ccpu_movem_hook_exit(struct cpu *cpu)
{
  TEST_CASE_END;
  exit(0);
}

static void test_ccpu_movem_hook_reset_cycles(struct cpu *cpu)
{
  tmp_cycles = cpu->cycle;
}

static void test_ccpu_movem_verify_test_dispatch(struct cpu *cpu)
{
  switch(test_num) {
  case 1:
    test_ccpu_movem_verify_test1(cpu);
    break;
  case 2:
    test_ccpu_movem_verify_test2(cpu);
    break;
  case 3:
    test_ccpu_movem_verify_test3(cpu);
    break;
  case 4:
    test_ccpu_movem_verify_test4(cpu);
    break;
  case 5:
    test_ccpu_movem_verify_test5(cpu);
    break;
  case 6:
    test_ccpu_movem_verify_test6(cpu);
    break;
  case 7:
    test_ccpu_movem_verify_test7(cpu);
    break;
  }
}

static void test_ccpu_movem_setup_test_dispatch(struct cpu *cpu)
{
  test_num++;
  switch(test_num) {
  case 1:
    test_ccpu_movem_setup_test1(cpu);
    break;
  case 2:
    test_ccpu_movem_setup_test2(cpu);
    break;
  case 3:
    test_ccpu_movem_setup_test3(cpu);
    break;
  case 4:
    test_ccpu_movem_setup_test4(cpu);
    break;
  case 5:
    test_ccpu_movem_setup_test5(cpu);
    break;
  case 6:
    test_ccpu_movem_setup_test6(cpu);
    break;
  case 7:
    test_ccpu_movem_setup_test7(cpu);
    break;
  }

  tmp_cycles = cpu->cycle;
}

void test_ccpu_movem_init()
{
  struct test_case *test_case;
  
  test_case = test_case_alloc("ccpu_movem");

  test_case->hooks[TEST_HOOK_INIT] = test_ccpu_movem_hook_init;
  test_case->hooks[TEST_HOOK_VERIFY_TEST] = test_ccpu_movem_verify_test_dispatch;
  test_case->hooks[TEST_HOOK_SETUP_TEST] = test_ccpu_movem_setup_test_dispatch;
  test_case->hooks[TEST_HOOK_EXIT] = test_ccpu_movem_hook_exit;
  
  test_case_register(test_case);
}
