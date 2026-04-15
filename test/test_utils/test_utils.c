#include "unity.h"
#include "utils.h"

void setUp(void) {}
void tearDown(void) {}

// ── MAX ──────────────────────────────────────────────────────────────────────

void test_max_second_larger(void) { TEST_ASSERT_EQUAL_INT(5, MAX(3, 5)); }
void test_max_first_larger(void) { TEST_ASSERT_EQUAL_INT(7, MAX(7, 2)); }
void test_max_equal(void) { TEST_ASSERT_EQUAL_INT(4, MAX(4, 4)); }
void test_max_negative(void) { TEST_ASSERT_EQUAL_INT(-1, MAX(-1, -5)); }
void test_max_zero_vs_negative(void) { TEST_ASSERT_EQUAL_INT(0, MAX(0, -3)); }

// ── MIN ──────────────────────────────────────────────────────────────────────

void test_min_second_smaller(void) { TEST_ASSERT_EQUAL_INT(1, MIN(3, 1)); }
void test_min_first_smaller(void) { TEST_ASSERT_EQUAL_INT(2, MIN(2, 9)); }
void test_min_equal(void) { TEST_ASSERT_EQUAL_INT(6, MIN(6, 6)); }
void test_min_negative(void) { TEST_ASSERT_EQUAL_INT(-8, MIN(-2, -8)); }
void test_min_zero_vs_positive(void) { TEST_ASSERT_EQUAL_INT(0, MIN(0, 5)); }

// ── Entry point ──────────────────────────────────────────────────────────────

void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_max_second_larger);
    RUN_TEST(test_max_first_larger);
    RUN_TEST(test_max_equal);
    RUN_TEST(test_max_negative);
    RUN_TEST(test_max_zero_vs_negative);
    RUN_TEST(test_min_second_smaller);
    RUN_TEST(test_min_first_smaller);
    RUN_TEST(test_min_equal);
    RUN_TEST(test_min_negative);
    RUN_TEST(test_min_zero_vs_positive);
    UNITY_END();
}
