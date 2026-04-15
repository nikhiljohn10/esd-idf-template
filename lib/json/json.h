#ifndef JSON_H
#define JSON_H

#include "dict.h"
#include <stddef.h>
#include <stdbool.h>

/**
 * Parse a JSON object string (e.g. {"key":"value","n":"42"}) into a dict.
 * Only flat objects with string values are supported.
 * Returns a new dict on success, NULL on error. Caller must dict_destroy().
 */
dict_t *json_parse(const char *json_str);

/**
 * Encode a dict into a JSON object string.
 * All values are emitted as strings. Writes up to buf_size bytes
 * (including null terminator) into buf.
 * Returns the number of bytes written (excluding '\0'), or -1 on error.
 */
int json_encode(dict_t *dict, char *buf, size_t buf_size);

#endif // JSON_H
