#include "json.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Minimal JSON parser – flat object with string values only
// ---------------------------------------------------------------------------

// Skip whitespace, return pointer to next non-space char
static const char *skip_ws(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

// Parse a JSON string token (starting at the opening '"').
// Writes the unescaped content into out (up to out_size-1 chars + '\0').
// Returns pointer past the closing '"', or NULL on error.
static const char *parse_string(const char *p, char *out, size_t out_size)
{
    if (*p != '"') return NULL;
    p++; // skip opening quote

    size_t i = 0;
    while (*p && *p != '"') {
        char c = *p++;
        if (c == '\\' && *p) {
            switch (*p) {
                case '"':  c = '"';  break;
                case '\\': c = '\\'; break;
                case '/':  c = '/';  break;
                case 'n':  c = '\n'; break;
                case 't':  c = '\t'; break;
                case 'r':  c = '\r'; break;
                case 'b':  c = '\b'; break;
                case 'f':  c = '\f'; break;
                default:   c = *p;   break;
            }
            p++;
        }
        if (i < out_size - 1) out[i++] = c;
    }
    if (*p != '"') return NULL;
    out[i] = '\0';
    return p + 1; // skip closing quote
}

// Parse a JSON number or literal (unquoted value) into a string
static const char *parse_value_raw(const char *p, char *out, size_t out_size)
{
    size_t i = 0;
    while (*p && *p != ',' && *p != '}' && *p != ' ' && *p != '\n' && *p != '\r' && *p != '\t') {
        if (i < out_size - 1) out[i++] = *p;
        p++;
    }
    out[i] = '\0';
    return p;
}

dict_t *json_parse(const char *json_str)
{
    if (!json_str) return NULL;

    const char *p = skip_ws(json_str);
    if (*p != '{') return NULL;
    p++;

    dict_t *d = dict_create();
    if (!d) return NULL;

    p = skip_ws(p);
    if (*p == '}') return d; // empty object

    while (*p) {
        p = skip_ws(p);
        if (*p != '"') goto fail;

        char key[DICT_KEY_MAX_LEN];
        p = parse_string(p, key, sizeof(key));
        if (!p) goto fail;

        p = skip_ws(p);
        if (*p != ':') goto fail;
        p++;
        p = skip_ws(p);

        char val[DICT_VAL_MAX_LEN];
        if (*p == '"') {
            p = parse_string(p, val, sizeof(val));
            if (!p) goto fail;
        } else {
            // number, bool, null — store as string
            p = parse_value_raw(p, val, sizeof(val));
        }

        dict_set(d, key, val);

        p = skip_ws(p);
        if (*p == ',') { p++; continue; }
        if (*p == '}') break;
        goto fail;
    }
    return d;

fail:
    dict_destroy(d);
    return NULL;
}

// ---------------------------------------------------------------------------
// JSON encoder
// ---------------------------------------------------------------------------

typedef struct {
    char  *buf;
    size_t size;
    size_t pos;
} encode_ctx_t;

static void enc_char(encode_ctx_t *ctx, char c)
{
    if (ctx->pos < ctx->size - 1) ctx->buf[ctx->pos] = c;
    ctx->pos++;
}

static void enc_str(encode_ctx_t *ctx, const char *s)
{
    while (*s) enc_char(ctx, *s++);
}

static void enc_json_string(encode_ctx_t *ctx, const char *s)
{
    enc_char(ctx, '"');
    while (*s) {
        switch (*s) {
            case '"':  enc_str(ctx, "\\\""); break;
            case '\\': enc_str(ctx, "\\\\"); break;
            case '\n': enc_str(ctx, "\\n");  break;
            case '\r': enc_str(ctx, "\\r");  break;
            case '\t': enc_str(ctx, "\\t");  break;
            default:   enc_char(ctx, *s);    break;
        }
        s++;
    }
    enc_char(ctx, '"');
}

static bool encode_iter(const char *key, const char *value, void *user_ctx)
{
    encode_ctx_t *ctx = (encode_ctx_t *)user_ctx;
    // prepend comma if not the first entry
    if (ctx->pos > 1) enc_char(ctx, ',');
    enc_json_string(ctx, key);
    enc_char(ctx, ':');
    enc_json_string(ctx, value);
    return true;
}

int json_encode(dict_t *dict, char *buf, size_t buf_size)
{
    if (!dict || !buf || buf_size < 3) return -1;

    encode_ctx_t ctx = {.buf = buf, .size = buf_size, .pos = 0};
    enc_char(&ctx, '{');
    dict_foreach(dict, encode_iter, &ctx);
    enc_char(&ctx, '}');

    // null-terminate
    size_t nul_pos = ctx.pos < buf_size ? ctx.pos : buf_size - 1;
    buf[nul_pos] = '\0';

    if (ctx.pos >= buf_size) return -1; // truncated
    return (int)ctx.pos;
}
