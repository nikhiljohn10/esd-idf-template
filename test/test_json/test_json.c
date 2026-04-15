#include "unity.h"
#include "json.h"
#include "dict.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

// ── json_parse: invalid inputs ────────────────────────────────────────────────

void test_json_parse_null_input(void)
{
    TEST_ASSERT_NULL(json_parse(NULL));
}

void test_json_parse_invalid_not_object(void)
{
    TEST_ASSERT_NULL(json_parse("not json"));
    TEST_ASSERT_NULL(json_parse("[1,2,3]"));
}

// ── json_parse: valid inputs ──────────────────────────────────────────────────

void test_json_parse_empty_object(void)
{
    dict_t *d = json_parse("{}");
    TEST_ASSERT_NOT_NULL(d);
    TEST_ASSERT_EQUAL_INT(0, dict_count(d));
    dict_destroy(d);
}

void test_json_parse_single_string_value(void)
{
    dict_t *d = json_parse("{\"key\":\"value\"}");
    TEST_ASSERT_NOT_NULL(d);
    char buf[64];
    TEST_ASSERT_TRUE(dict_get(d, "key", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("value", buf);
    dict_destroy(d);
}

void test_json_parse_multiple_pairs(void)
{
    dict_t *d = json_parse("{\"a\":\"1\",\"b\":\"2\",\"c\":\"3\"}");
    TEST_ASSERT_NOT_NULL(d);
    TEST_ASSERT_EQUAL_INT(3, dict_count(d));
    char buf[64];
    dict_get(d, "a", buf, sizeof(buf)); TEST_ASSERT_EQUAL_STRING("1", buf);
    dict_get(d, "b", buf, sizeof(buf)); TEST_ASSERT_EQUAL_STRING("2", buf);
    dict_get(d, "c", buf, sizeof(buf)); TEST_ASSERT_EQUAL_STRING("3", buf);
    dict_destroy(d);
}

void test_json_parse_unquoted_number_stored_as_string(void)
{
    dict_t *d = json_parse("{\"score\":42}");
    TEST_ASSERT_NOT_NULL(d);
    char buf[16];
    TEST_ASSERT_TRUE(dict_get(d, "score", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("42", buf);
    dict_destroy(d);
}

void test_json_parse_bool_literals_stored_as_string(void)
{
    dict_t *d = json_parse("{\"on\":true,\"off\":false}");
    TEST_ASSERT_NOT_NULL(d);
    char buf[16];
    dict_get(d, "on",  buf, sizeof(buf)); TEST_ASSERT_EQUAL_STRING("true",  buf);
    dict_get(d, "off", buf, sizeof(buf)); TEST_ASSERT_EQUAL_STRING("false", buf);
    dict_destroy(d);
}

void test_json_parse_escape_sequences_in_value(void)
{
    // JSON: {"msg":"say \"hello\""}
    dict_t *d = json_parse("{\"msg\":\"say \\\"hello\\\"\"}");
    TEST_ASSERT_NOT_NULL(d);
    char buf[64];
    TEST_ASSERT_TRUE(dict_get(d, "msg", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("say \"hello\"", buf);
    dict_destroy(d);
}

void test_json_parse_whitespace_around_tokens(void)
{
    dict_t *d = json_parse("{ \"k\" : \"v\" }");
    TEST_ASSERT_NOT_NULL(d);
    char buf[16];
    TEST_ASSERT_TRUE(dict_get(d, "k", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("v", buf);
    dict_destroy(d);
}

// ── json_encode ───────────────────────────────────────────────────────────────

void test_json_encode_single_pair(void)
{
    dict_t *d = dict_create();
    dict_set(d, "key", "val");
    char buf[64];
    int n = json_encode(d, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, n);
    // Re-parse to verify content — avoids dependence on key-insertion order
    dict_t *d2 = json_parse(buf);
    TEST_ASSERT_NOT_NULL(d2);
    char vbuf[32];
    TEST_ASSERT_TRUE(dict_get(d2, "key", vbuf, sizeof(vbuf)));
    TEST_ASSERT_EQUAL_STRING("val", vbuf);
    dict_destroy(d);
    dict_destroy(d2);
}

void test_json_encode_returns_negative_on_small_buf(void)
{
    dict_t *d = dict_create();
    dict_set(d, "key", "val");
    char tiny[4];
    TEST_ASSERT_EQUAL_INT(-1, json_encode(d, tiny, sizeof(tiny)));
    dict_destroy(d);
}

void test_json_encode_escapes_quotes_in_value(void)
{
    dict_t *d = dict_create();
    dict_set(d, "k", "a\"b");
    char buf[64];
    json_encode(d, buf, sizeof(buf));
    // Re-parse: value must round-trip correctly
    dict_t *d2 = json_parse(buf);
    TEST_ASSERT_NOT_NULL(d2);
    char vbuf[32];
    dict_get(d2, "k", vbuf, sizeof(vbuf));
    TEST_ASSERT_EQUAL_STRING("a\"b", vbuf);
    dict_destroy(d);
    dict_destroy(d2);
}

// ── roundtrip ─────────────────────────────────────────────────────────────────

void test_json_roundtrip_preserves_all_values(void)
{
    dict_t *orig = json_parse("{\"ip\":\"1.2.3.4\",\"host\":\"esp32\"}");
    TEST_ASSERT_NOT_NULL(orig);
    char buf[256];
    TEST_ASSERT_GREATER_THAN(0, json_encode(orig, buf, sizeof(buf)));
    dict_t *copy = json_parse(buf);
    TEST_ASSERT_NOT_NULL(copy);
    char vbuf[64];
    dict_get(copy, "ip",   vbuf, sizeof(vbuf)); TEST_ASSERT_EQUAL_STRING("1.2.3.4", vbuf);
    dict_get(copy, "host", vbuf, sizeof(vbuf)); TEST_ASSERT_EQUAL_STRING("esp32",   vbuf);
    dict_destroy(orig);
    dict_destroy(copy);
}

// ── Entry point ───────────────────────────────────────────────────────────────

void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_json_parse_null_input);
    RUN_TEST(test_json_parse_invalid_not_object);
    RUN_TEST(test_json_parse_empty_object);
    RUN_TEST(test_json_parse_single_string_value);
    RUN_TEST(test_json_parse_multiple_pairs);
    RUN_TEST(test_json_parse_unquoted_number_stored_as_string);
    RUN_TEST(test_json_parse_bool_literals_stored_as_string);
    RUN_TEST(test_json_parse_escape_sequences_in_value);
    RUN_TEST(test_json_parse_whitespace_around_tokens);
    RUN_TEST(test_json_encode_single_pair);
    RUN_TEST(test_json_encode_returns_negative_on_small_buf);
    RUN_TEST(test_json_encode_escapes_quotes_in_value);
    RUN_TEST(test_json_roundtrip_preserves_all_values);
    UNITY_END();
}
