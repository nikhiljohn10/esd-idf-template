#include "unity.h"
#include "dict.h"
#include <string.h>

static dict_t *d;

void setUp(void)    { d = dict_create(); }
void tearDown(void) { dict_destroy(d); d = NULL; }

// ── create ───────────────────────────────────────────────────────────────────

void test_dict_create_not_null(void)
{
    TEST_ASSERT_NOT_NULL(d);
}

// ── count ────────────────────────────────────────────────────────────────────

void test_dict_empty_count_is_zero(void)
{
    TEST_ASSERT_EQUAL_INT(0, dict_count(d));
}

void test_dict_count_increments_on_set(void)
{
    dict_set(d, "a", "1");
    TEST_ASSERT_EQUAL_INT(1, dict_count(d));
    dict_set(d, "b", "2");
    TEST_ASSERT_EQUAL_INT(2, dict_count(d));
}

void test_dict_count_unchanged_on_overwrite(void)
{
    dict_set(d, "key", "v1");
    dict_set(d, "key", "v2");
    TEST_ASSERT_EQUAL_INT(1, dict_count(d));
}

void test_dict_count_decrements_on_delete(void)
{
    dict_set(d, "x", "1");
    dict_set(d, "y", "2");
    dict_delete(d, "x");
    TEST_ASSERT_EQUAL_INT(1, dict_count(d));
}

// ── set / get ────────────────────────────────────────────────────────────────

void test_dict_set_returns_true(void)
{
    TEST_ASSERT_TRUE(dict_set(d, "key", "value"));
}

void test_dict_get_returns_true_on_hit(void)
{
    dict_set(d, "k", "v");
    char buf[64];
    TEST_ASSERT_TRUE(dict_get(d, "k", buf, sizeof(buf)));
}

void test_dict_get_correct_value(void)
{
    dict_set(d, "name", "alice");
    char buf[64];
    dict_get(d, "name", buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("alice", buf);
}

void test_dict_get_missing_returns_false(void)
{
    char buf[64];
    TEST_ASSERT_FALSE(dict_get(d, "nope", buf, sizeof(buf)));
}

void test_dict_set_overwrites_value(void)
{
    dict_set(d, "k", "old");
    dict_set(d, "k", "new");
    char buf[64];
    dict_get(d, "k", buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("new", buf);
}

// ── delete ───────────────────────────────────────────────────────────────────

void test_dict_delete_returns_true_on_hit(void)
{
    dict_set(d, "k", "v");
    TEST_ASSERT_TRUE(dict_delete(d, "k"));
}

void test_dict_delete_removes_entry(void)
{
    dict_set(d, "k", "v");
    dict_delete(d, "k");
    char buf[64];
    TEST_ASSERT_FALSE(dict_get(d, "k", buf, sizeof(buf)));
}

void test_dict_delete_missing_returns_false(void)
{
    TEST_ASSERT_FALSE(dict_delete(d, "ghost"));
}

// ── foreach ──────────────────────────────────────────────────────────────────

static bool count_items(const char *key, const char *value, void *ctx)
{
    (void)key; (void)value;
    (*(int *)ctx)++;
    return true;
}

void test_dict_foreach_visits_all_entries(void)
{
    dict_set(d, "a", "1");
    dict_set(d, "b", "2");
    dict_set(d, "c", "3");
    int count = 0;
    dict_foreach(d, count_items, &count);
    TEST_ASSERT_EQUAL_INT(3, count);
}

static bool stop_early(const char *key, const char *value, void *ctx)
{
    (void)key; (void)value;
    (*(int *)ctx)++;
    return false;
}

void test_dict_foreach_stops_on_false(void)
{
    dict_set(d, "a", "1");
    dict_set(d, "b", "2");
    dict_set(d, "c", "3");
    int count = 0;
    dict_foreach(d, stop_early, &count);
    TEST_ASSERT_EQUAL_INT(1, count);
}

// ── null safety ──────────────────────────────────────────────────────────────

void test_dict_null_safety(void)
{
    char buf[64];
    TEST_ASSERT_FALSE(dict_get(NULL, "k", buf, sizeof(buf)));
    TEST_ASSERT_FALSE(dict_set(NULL, "k", "v"));
    TEST_ASSERT_FALSE(dict_delete(NULL, "k"));
    TEST_ASSERT_EQUAL_INT(-1, dict_count(NULL));
    dict_destroy(NULL); // must not crash
}

// ── truncation ───────────────────────────────────────────────────────────────

void test_dict_get_truncates_to_buf_size(void)
{
    dict_set(d, "k", "hello");
    char small_buf[3]; // holds "he\0"
    dict_get(d, "k", small_buf, sizeof(small_buf));
    TEST_ASSERT_EQUAL_STRING("he", small_buf);
}

// ── Entry point ──────────────────────────────────────────────────────────────

void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_dict_create_not_null);
    RUN_TEST(test_dict_empty_count_is_zero);
    RUN_TEST(test_dict_count_increments_on_set);
    RUN_TEST(test_dict_count_unchanged_on_overwrite);
    RUN_TEST(test_dict_count_decrements_on_delete);
    RUN_TEST(test_dict_set_returns_true);
    RUN_TEST(test_dict_get_returns_true_on_hit);
    RUN_TEST(test_dict_get_correct_value);
    RUN_TEST(test_dict_get_missing_returns_false);
    RUN_TEST(test_dict_set_overwrites_value);
    RUN_TEST(test_dict_delete_returns_true_on_hit);
    RUN_TEST(test_dict_delete_removes_entry);
    RUN_TEST(test_dict_delete_missing_returns_false);
    RUN_TEST(test_dict_foreach_visits_all_entries);
    RUN_TEST(test_dict_foreach_stops_on_false);
    RUN_TEST(test_dict_null_safety);
    RUN_TEST(test_dict_get_truncates_to_buf_size);
    UNITY_END();
}
