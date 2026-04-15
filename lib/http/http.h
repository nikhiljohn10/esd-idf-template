#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

/**
 * HTTP response returned by all request functions.
 * Caller must free(body) when done.
 */
typedef struct {
    uint8_t *body;       // heap-allocated, null-terminated (NULL on error)
    size_t   body_len;   // bytes in body excluding '\0'
    int      status;     // HTTP status code (e.g. 200)
} http_response_t;

/**
 * All methods share the same signature:
 *   url       – the full URL (http:// or https://)
 *   body      – request body (may be NULL for methods that have no body)
 *   body_len  – length of request body
 *   resp      – output struct, caller must free(resp->body) on success
 *
 * Returns ESP_OK on success (even if HTTP status is 4xx/5xx).
 */

esp_err_t http_get    (const char *url, const uint8_t *body, size_t body_len, http_response_t *resp);
esp_err_t http_post   (const char *url, const uint8_t *body, size_t body_len, http_response_t *resp);
esp_err_t http_put    (const char *url, const uint8_t *body, size_t body_len, http_response_t *resp);
esp_err_t http_patch  (const char *url, const uint8_t *body, size_t body_len, http_response_t *resp);
esp_err_t http_delete (const char *url, const uint8_t *body, size_t body_len, http_response_t *resp);
esp_err_t http_head   (const char *url, const uint8_t *body, size_t body_len, http_response_t *resp);
esp_err_t http_options(const char *url, const uint8_t *body, size_t body_len, http_response_t *resp);

#endif // HTTP_H
