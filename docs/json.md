# json

Flat JSON object parser and encoder backed by `dict_t`.

**Header:** `lib/json/json.h`  
**Depends on:** `dict`

---

## Overview

Handles flat JSON objects (no nesting, no arrays).
All values — strings, numbers, booleans, and `null` — are stored as `char *` strings inside the returned `dict_t`.

| JSON value | Stored as |
|------------|-----------|
| `"hello"` | `hello` |
| `42` | `42` |
| `true` / `false` | `true` / `false` |
| `null` | `null` |

Escape sequences (`\"`, `\\`, `\n`, `\r`, `\t`) inside string values are decoded during parsing and re-encoded when writing.

---

## API

### `dict_t *json_parse(const char *json_str)`

Parses a JSON object string into a `dict_t`.

Returns a new `dict_t` on success. Returns `NULL` if:
- `json_str` is `NULL`
- the input is not a JSON object (e.g. an array, a bare value, or invalid JSON)

The caller must `dict_destroy()` the returned dict.

```c
dict_t *d = json_parse("{\"ip\":\"1.2.3.4\",\"ok\":true}");
if (d) {
    char ip[32];
    dict_get(d, "ip", ip, sizeof(ip)); // "1.2.3.4"
    dict_get(d, "ok", ip, sizeof(ip)); // "true"
    dict_destroy(d);
}
```

Whitespace around keys, colons, and commas is ignored:

```c
dict_t *d = json_parse("{ \"key\" : \"value\" }");
```

---

### `int json_encode(dict_t *dict, char *buf, size_t buf_size)`

Serialises all entries in `dict` into a JSON object string written into `buf`.

- All values are emitted as JSON strings (quoted).
- Quote characters inside values are escaped as `\"`.
- Returns the number of bytes written (excluding the null terminator), or `-1` if the buffer is too small.

```c
dict_t *d = dict_create();
dict_set(d, "status", "ok");
dict_set(d, "count",  "7");

char buf[128];
int n = json_encode(d, buf, sizeof(buf));
if (n > 0) {
    printf("%s\n", buf); // {"status":"ok","count":"7"}
}
dict_destroy(d);
```

If `-1` is returned, `buf` content is undefined — increase `buf_size` and retry.

---

## Roundtrip example

```c
#include "json.h"
#include "dict.h"
#include <stdio.h>

void roundtrip_example(void) {
    // Parse incoming JSON
    dict_t *d = json_parse("{\"name\":\"ESP32\",\"temp\":\"28\"}");

    // Modify a value
    dict_set(d, "temp", "30");

    // Re-encode
    char out[256];
    if (json_encode(d, out, sizeof(out)) > 0) {
        printf("%s\n", out);
        // {"name":"ESP32","temp":"30"}
    }

    dict_destroy(d);
}
```

---

## HTTP response parsing example

```c
#include "http.h"
#include "json.h"
#include "dict.h"

void fetch_ip(void) {
    http_response_t resp;
    if (http_get("https://api.ipify.org?format=json", NULL, 0, &resp) == ESP_OK) {
        dict_t *d = json_parse((const char *)resp.body);
        if (d) {
            char ip[64];
            if (dict_get(d, "ip", ip, sizeof(ip))) {
                printf("Public IP: %s\n", ip);
            }
            dict_destroy(d);
        }
        free(resp.body);
    }
}
```

---

## Limitations

- Flat objects only — nested objects and arrays are not supported.
- Key and value lengths are bounded by `DICT_KEY_MAX_LEN` (64) and `DICT_VAL_MAX_LEN` (256).
- Key insertion order is not preserved in the encoded output.
