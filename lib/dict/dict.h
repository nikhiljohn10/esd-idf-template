#ifndef DICT_H
#define DICT_H

#include <stdbool.h>
#include <stddef.h>

// Total bucket count (must be power of 2)
#define DICT_BUCKET_COUNT   64
// Number of lock stripes (must divide DICT_BUCKET_COUNT evenly)
#define DICT_STRIPE_COUNT   8
// Maximum key / value length including null terminator
#define DICT_KEY_MAX_LEN    64
#define DICT_VAL_MAX_LEN    256

typedef struct dict_t dict_t;

/**
 * Create a new concurrent hashtable. Returns NULL on failure.
 * Must be freed with dict_destroy().
 */
dict_t *dict_create(void);

/**
 * Destroy the dict and free all memory. Safe to call with NULL.
 */
void dict_destroy(dict_t *dict);

/**
 * Insert or update a string value for the given key.
 * Both key and value are copied. Returns true on success.
 */
bool dict_set(dict_t *dict, const char *key, const char *value);

/**
 * Look up a key. On success writes the value into out_buf (up to buf_size
 * bytes including null terminator) and returns true. Returns false if not
 * found or on error.
 */
bool dict_get(dict_t *dict, const char *key, char *out_buf, size_t buf_size);

/**
 * Remove the entry with the given key.
 * Returns true if removed, false if not found.
 */
bool dict_delete(dict_t *dict, const char *key);

/**
 * Return the total number of entries stored, or -1 on error.
 */
int dict_count(dict_t *dict);

/**
 * Iteration callback. Return true to continue, false to stop.
 */
typedef bool (*dict_iter_fn)(const char *key, const char *value, void *ctx);

/**
 * Iterate over all entries. Acquires each stripe lock in turn.
 * The callback must not call dict_set/dict_delete/dict_destroy.
 */
void dict_foreach(dict_t *dict, dict_iter_fn fn, void *ctx);

#endif // DICT_H
