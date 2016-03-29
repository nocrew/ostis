#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#define EXPECT_MSG_INT(v1, v2, name) printf(" \t[FAIL]\n - Expected %s to eq %d - Got %d instead\n", name, v2, v1);
#define EXPECT_MSG_INT64(v1, v2, name) printf(" \t[FAIL]\n - Expected %s to eq %ld - Got %ld instead\n", name, v2, v1);
#define EXPECT_MSG_ADDR(v1, v2, name) printf(" \t[FAIL]\n - Expected %s to eq $%06x - Got $%06x instead\n", name, v2, v1);
#define EXPECT_MSG_BYTE(v1, v2, name) printf(" \t[FAIL]\n - Expected %s to eq $%02x - Got $%02x instead\n", name, v2, v1);
#define EXPECT_MSG_WORD(v1, v2, name) printf(" \t[FAIL]\n - Expected %s to eq $%04x - Got $%04x instead\n", name, v2, v1);
#define EXPECT_MSG_LONG(v1, v2, name) printf(" \t[FAIL]\n - Expected %s to eq $%08x - Got $%08x instead\n", name, v2, v1);

static int test_main_current_test = 0;

#define TEST_CASE_START(name, text) \
  do { \
    printf("------------------------------------------------\n"); \
    printf("Case %s - %s\n\n", name, text); \
  } while(0);

#define TEST_CASE_END     printf("------------------------------------------------\n\n");

#define EXPECT_EQ(type, v1, v2, name) \
  if(!test_main_current_test) { \
    printf("TEST-SETUP-ERROR! TEST_START was not provided or not given a proper test number\n"); \
    exit(-2); \
  } \
  if(v1 != v2) { \
    EXPECT_MSG_ ## type(v1, v2, name); \
    exit(-1); \
  }

#define TEST_START(tnum, text) \
  do { \
    test_main_current_test = tnum; \
    printf("Test %d - %s", test_main_current_test, text); \
  } while(0);

#define TEST_END_OK \
  do {                          \
    test_main_current_test = 0; \
    printf(" \t[SUCCESS]\n"); \
  } while(0);

#endif
