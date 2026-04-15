# http

HTTP client supporting all standard methods, with TLS via the ESP certificate bundle.

**Header:** `lib/http/http.h`  
**Depends on:** `net` (WiFi must be connected before use)

---

## Response type

```c
typedef struct {
    uint8_t *body;      // heap-allocated, null-terminated (NULL on transport error)
    size_t   body_len;  // byte count excluding '\0'
    int      status;    // HTTP status code (200, 404, etc.)
} http_response_t;
```

The caller is responsible for calling `free(resp.body)` after use.

---

## API

All functions share the same signature:

```c
esp_err_t http_METHOD(
    const char    *url,       // full URL including scheme
    const uint8_t *body,      // request body (NULL if none)
    size_t         body_len,  // length of request body
    http_response_t *resp     // output — caller must free(resp->body)
);
```

Returns `ESP_OK` if the transport succeeded (regardless of HTTP status code).
Returns an `esp_err_t` error code if the connection or transfer failed.

### Available methods

| Function       | HTTP method |
| -------------- | ----------- |
| `http_get`     | GET         |
| `http_post`    | POST        |
| `http_put`     | PUT         |
| `http_patch`   | PATCH       |
| `http_delete`  | DELETE      |
| `http_head`    | HEAD        |
| `http_options` | OPTIONS     |

---

## Examples

### GET request

```c
#include "http.h"
#include <stdio.h>

void get_example(void) {
    http_response_t resp;
    if (http_get("https://api.ipify.org?format=json", NULL, 0, &resp) == ESP_OK) {
        printf("Status: %d\n", resp.status);
        printf("Body:   %s\n", resp.body);
        free(resp.body);
    }
}
```

### POST request with JSON body

```c
#include "http.h"
#include "json.h"
#include "dict.h"

void post_example(void) {
    // Build request body
    dict_t *payload = dict_create();
    dict_set(payload, "device", "esp32");
    dict_set(payload, "temp",   "28");

    char body[256];
    json_encode(payload, body, sizeof(body));
    dict_destroy(payload);

    // Send
    http_response_t resp;
    esp_err_t err = http_post(
        "https://example.com/api/data",
        (const uint8_t *)body,
        strlen(body),
        &resp
    );

    if (err == ESP_OK) {
        printf("HTTP %d: %s\n", resp.status, resp.body);
        free(resp.body);
    }
}
```

### Parse JSON response

```c
#include "http.h"
#include "json.h"

void parse_response(void) {
    http_response_t resp;
    if (http_get("https://httpbin.org/json", NULL, 0, &resp) != ESP_OK) return;

    dict_t *d = json_parse((const char *)resp.body);
    free(resp.body);

    if (d) {
        char val[128];
        if (dict_get(d, "slideshow", val, sizeof(val))) {
            printf("slideshow: %s\n", val);
        }
        dict_destroy(d);
    }
}
```

---

## Notes

- HTTPS is supported out of the box via `esp_crt_bundle_attach` — no manual certificate management required.
- `resp.body` is `NULL` if the transport failed. Always check the return value before using it.
- Response bodies are heap-allocated; always `free(resp.body)` to avoid memory leaks.
- WiFi must be connected and (for HTTPS) `setup_network()` must have been called first to sync the system clock for certificate validation.
- There is no timeout configured by default; use `esp_http_client_set_timeout_ms()` if you need one.
