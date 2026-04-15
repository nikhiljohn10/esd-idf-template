# dict

Thread-safe string key-value store backed by a concurrent hashtable.

**Header:** `lib/dict/dict.h`

---

## Overview

| Property | Value |
|----------|-------|
| Buckets | 64 (djb2 hash) |
| Lock stripes | 8 FreeRTOS mutexes |
| Max key length | 63 chars + `\0` |
| Max value length | 255 chars + `\0` |
| Value type | `const char *` (copied on write) |

Reads and writes to different hash stripes can proceed concurrently. All values are stored and returned as null-terminated strings.

---

## API

### `dict_t *dict_create(void)`

Allocates a new dict. Returns `NULL` on allocation failure.
Must be freed with `dict_destroy()`.

```c
dict_t *d = dict_create();
```

---

### `void dict_destroy(dict_t *dict)`

Frees all memory. Safe to call with `NULL`.

```c
dict_destroy(d);
```

---

### `bool dict_set(dict_t *dict, const char *key, const char *value)`

Inserts or overwrites `key` with `value`. Both are copied internally.
Returns `true` on success, `false` on allocation failure or if either argument is `NULL`.

```c
dict_set(d, "ssid", "my-network");
dict_set(d, "retries", "3");        // values are always strings
```

---

### `bool dict_get(dict_t *dict, const char *key, char *out_buf, size_t buf_size)`

Copies the value for `key` into `out_buf` (up to `buf_size` bytes including `\0`).
Returns `true` if found, `false` if the key does not exist or any argument is `NULL`.

```c
char ssid[64];
if (dict_get(d, "ssid", ssid, sizeof(ssid))) {
    printf("SSID: %s\n", ssid);
}
```

If `buf_size` is smaller than the stored value, the result is truncated and null-terminated.

---

### `bool dict_delete(dict_t *dict, const char *key)`

Removes the entry for `key`. Returns `true` if it existed and was removed, `false` otherwise.

```c
dict_delete(d, "temp_key");
```

---

### `int dict_count(dict_t *dict)`

Returns the total number of entries, or `-1` on error.

```c
printf("Entries: %d\n", dict_count(d));
```

---

### `void dict_foreach(dict_t *dict, dict_iter_fn fn, void *ctx)`

Iterates over all entries, calling `fn(key, value, ctx)` for each.
Return `true` from the callback to continue, `false` to stop early.

The callback **must not** call `dict_set`, `dict_delete`, or `dict_destroy` — doing so will deadlock.

```c
typedef bool (*dict_iter_fn)(const char *key, const char *value, void *ctx);
```

**Example — print all entries:**

```c
static bool print_entry(const char *key, const char *value, void *ctx) {
    printf("  %s = %s\n", key, value);
    return true; // continue
}

dict_foreach(d, print_entry, NULL);
```

**Example — find first entry whose value matches:**

```c
static bool find_value(const char *key, const char *value, void *ctx) {
    if (strcmp(value, (const char *)ctx) == 0) {
        printf("Found key: %s\n", key);
        return false; // stop
    }
    return true;
}

dict_foreach(d, find_value, "target-value");
```

---

## Full example

```c
#include "dict.h"
#include <stdio.h>

void example(void) {
    dict_t *d = dict_create();

    dict_set(d, "host", "esp32.local");
    dict_set(d, "port", "80");
    dict_set(d, "path", "/api/status");

    char host[64];
    if (dict_get(d, "host", host, sizeof(host))) {
        printf("Host: %s\n", host);
    }

    printf("Count: %d\n", dict_count(d)); // 3

    dict_delete(d, "path");
    printf("Count: %d\n", dict_count(d)); // 2

    dict_destroy(d);
}
```
