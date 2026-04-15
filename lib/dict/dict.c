#include "dict.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdlib.h>
#include <string.h>

// ---- internal types -------------------------------------------------------

typedef struct dict_node {
    char                key[DICT_KEY_MAX_LEN];
    char                value[DICT_VAL_MAX_LEN];
    struct dict_node   *next;
} dict_node_t;

typedef struct {
    dict_node_t *head;
} dict_bucket_t;

struct dict_t {
    dict_bucket_t       buckets[DICT_BUCKET_COUNT];
    SemaphoreHandle_t   stripe_locks[DICT_STRIPE_COUNT];
};

// ---- helpers --------------------------------------------------------------

// djb2 hash → bucket index
static unsigned int hash_key(const char *key)
{
    unsigned int h = 5381;
    while (*key) {
        h = ((h << 5) + h) + (unsigned char)*key++;
    }
    return h & (DICT_BUCKET_COUNT - 1);
}

static int stripe_of(unsigned int bucket)
{
    return (int)(bucket / (DICT_BUCKET_COUNT / DICT_STRIPE_COUNT));
}

static void lock_stripe(dict_t *d, int stripe)
{
    xSemaphoreTake(d->stripe_locks[stripe], portMAX_DELAY);
}

static void unlock_stripe(dict_t *d, int stripe)
{
    xSemaphoreGive(d->stripe_locks[stripe]);
}

// ---- public API -----------------------------------------------------------

dict_t *dict_create(void)
{
    dict_t *d = calloc(1, sizeof(dict_t));
    if (!d) return NULL;

    for (int i = 0; i < DICT_STRIPE_COUNT; i++) {
        d->stripe_locks[i] = xSemaphoreCreateMutex();
        if (!d->stripe_locks[i]) {
            for (int j = 0; j < i; j++) vSemaphoreDelete(d->stripe_locks[j]);
            free(d);
            return NULL;
        }
    }
    return d;
}

void dict_destroy(dict_t *d)
{
    if (!d) return;
    for (unsigned int b = 0; b < DICT_BUCKET_COUNT; b++) {
        dict_node_t *n = d->buckets[b].head;
        while (n) {
            dict_node_t *tmp = n;
            n = n->next;
            free(tmp);
        }
    }
    for (int i = 0; i < DICT_STRIPE_COUNT; i++) {
        vSemaphoreDelete(d->stripe_locks[i]);
    }
    free(d);
}

bool dict_set(dict_t *d, const char *key, const char *value)
{
    if (!d || !key || !value) return false;

    unsigned int bucket = hash_key(key);
    int stripe = stripe_of(bucket);
    lock_stripe(d, stripe);

    // update existing
    for (dict_node_t *n = d->buckets[bucket].head; n; n = n->next) {
        if (strncmp(n->key, key, DICT_KEY_MAX_LEN) == 0) {
            strncpy(n->value, value, DICT_VAL_MAX_LEN - 1);
            n->value[DICT_VAL_MAX_LEN - 1] = '\0';
            unlock_stripe(d, stripe);
            return true;
        }
    }

    // insert new node at head
    dict_node_t *node = malloc(sizeof(dict_node_t));
    if (!node) { unlock_stripe(d, stripe); return false; }
    strncpy(node->key, key, DICT_KEY_MAX_LEN - 1);
    node->key[DICT_KEY_MAX_LEN - 1] = '\0';
    strncpy(node->value, value, DICT_VAL_MAX_LEN - 1);
    node->value[DICT_VAL_MAX_LEN - 1] = '\0';
    node->next = d->buckets[bucket].head;
    d->buckets[bucket].head = node;

    unlock_stripe(d, stripe);
    return true;
}

bool dict_get(dict_t *d, const char *key, char *out_buf, size_t buf_size)
{
    if (!d || !key || !out_buf || buf_size == 0) return false;

    unsigned int bucket = hash_key(key);
    int stripe = stripe_of(bucket);
    lock_stripe(d, stripe);

    for (dict_node_t *n = d->buckets[bucket].head; n; n = n->next) {
        if (strncmp(n->key, key, DICT_KEY_MAX_LEN) == 0) {
            strncpy(out_buf, n->value, buf_size - 1);
            out_buf[buf_size - 1] = '\0';
            unlock_stripe(d, stripe);
            return true;
        }
    }

    unlock_stripe(d, stripe);
    return false;
}

bool dict_delete(dict_t *d, const char *key)
{
    if (!d || !key) return false;

    unsigned int bucket = hash_key(key);
    int stripe = stripe_of(bucket);
    lock_stripe(d, stripe);

    dict_node_t **pp = &d->buckets[bucket].head;
    while (*pp) {
        if (strncmp((*pp)->key, key, DICT_KEY_MAX_LEN) == 0) {
            dict_node_t *victim = *pp;
            *pp = victim->next;
            free(victim);
            unlock_stripe(d, stripe);
            return true;
        }
        pp = &(*pp)->next;
    }

    unlock_stripe(d, stripe);
    return false;
}

int dict_count(dict_t *d)
{
    if (!d) return -1;
    int total = 0;
    for (int s = 0; s < DICT_STRIPE_COUNT; s++) {
        lock_stripe(d, s);
        unsigned int start = (unsigned int)s * (DICT_BUCKET_COUNT / DICT_STRIPE_COUNT);
        unsigned int end   = start + (DICT_BUCKET_COUNT / DICT_STRIPE_COUNT);
        for (unsigned int b = start; b < end; b++) {
            for (dict_node_t *n = d->buckets[b].head; n; n = n->next)
                total++;
        }
        unlock_stripe(d, s);
    }
    return total;
}

void dict_foreach(dict_t *d, dict_iter_fn fn, void *ctx)
{
    if (!d || !fn) return;
    for (int s = 0; s < DICT_STRIPE_COUNT; s++) {
        lock_stripe(d, s);
        unsigned int start = (unsigned int)s * (DICT_BUCKET_COUNT / DICT_STRIPE_COUNT);
        unsigned int end   = start + (DICT_BUCKET_COUNT / DICT_STRIPE_COUNT);
        bool keep_going = true;
        for (unsigned int b = start; b < end && keep_going; b++) {
            for (dict_node_t *n = d->buckets[b].head; n && keep_going; n = n->next) {
                keep_going = fn(n->key, n->value, ctx);
            }
        }
        unlock_stripe(d, s);
        if (!keep_going) break;
    }
}
