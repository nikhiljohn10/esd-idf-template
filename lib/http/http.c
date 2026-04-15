#include "http.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "http";

// ---------------------------------------------------------------------------
// Internal accumulating event handler
// ---------------------------------------------------------------------------

typedef struct {
    uint8_t *buffer;
    size_t   len;
} resp_accum_t;

static esp_err_t event_handler(esp_http_client_event_t *evt)
{
    resp_accum_t *acc = (resp_accum_t *)evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA && acc && evt->data_len > 0) {
        uint8_t *new_buf = realloc(acc->buffer, acc->len + (size_t)evt->data_len + 1);
        if (!new_buf) return ESP_ERR_NO_MEM;
        acc->buffer = new_buf;
        memcpy(acc->buffer + acc->len, evt->data, (size_t)evt->data_len);
        acc->len += (size_t)evt->data_len;
    }
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Method-agnostic core
// ---------------------------------------------------------------------------

static esp_err_t http_request(esp_http_client_method_t method,
                              const char *url,
                              const uint8_t *body,
                              size_t body_len,
                              http_response_t *resp)
{
    if (!url || !resp) return ESP_ERR_INVALID_ARG;

    resp->body     = NULL;
    resp->body_len = 0;
    resp->status   = 0;

    resp_accum_t acc = {.buffer = NULL, .len = 0};

    esp_http_client_config_t cfg = {
        .url               = url,
        .method            = method,
        .event_handler     = event_handler,
        .user_data         = &acc,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return ESP_FAIL;

    if (body && body_len > 0) {
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, (const char *)body, (int)body_len);
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        if (acc.buffer) acc.buffer[acc.len] = '\0';
        resp->body     = acc.buffer;
        resp->body_len = acc.len;
        resp->status   = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "%s -> %d bytes, status %d",
                 url, (int)acc.len, resp->status);
    } else {
        ESP_LOGE(TAG, "request failed: %s", esp_err_to_name(err));
        free(acc.buffer);
    }

    esp_http_client_cleanup(client);
    return err;
}

// ---------------------------------------------------------------------------
// Public wrappers
// ---------------------------------------------------------------------------

esp_err_t http_get(const char *url, const uint8_t *body, size_t body_len, http_response_t *resp)
{
    return http_request(HTTP_METHOD_GET, url, body, body_len, resp);
}

esp_err_t http_post(const char *url, const uint8_t *body, size_t body_len, http_response_t *resp)
{
    return http_request(HTTP_METHOD_POST, url, body, body_len, resp);
}

esp_err_t http_put(const char *url, const uint8_t *body, size_t body_len, http_response_t *resp)
{
    return http_request(HTTP_METHOD_PUT, url, body, body_len, resp);
}

esp_err_t http_patch(const char *url, const uint8_t *body, size_t body_len, http_response_t *resp)
{
    return http_request(HTTP_METHOD_PATCH, url, body, body_len, resp);
}

esp_err_t http_delete(const char *url, const uint8_t *body, size_t body_len, http_response_t *resp)
{
    return http_request(HTTP_METHOD_DELETE, url, body, body_len, resp);
}

esp_err_t http_head(const char *url, const uint8_t *body, size_t body_len, http_response_t *resp)
{
    return http_request(HTTP_METHOD_HEAD, url, body, body_len, resp);
}

esp_err_t http_options(const char *url, const uint8_t *body, size_t body_len, http_response_t *resp)
{
    return http_request(HTTP_METHOD_OPTIONS, url, body, body_len, resp);
}


