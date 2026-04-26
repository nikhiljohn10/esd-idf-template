/**
 * @file net.c
 * @brief Resilient WiFi + NTP network library for ESP32 (ESP-IDF 5.x).
 *
 * ## Design
 *
 * net_start() stores credentials and spawns two FreeRTOS tasks:
 *
 *   wifi_init_task  — Performs one-time WiFi hardware initialisation,
 *                     registers the shared event handler, starts the WiFi
 *                     driver, then exits.  All subsequent WiFi lifecycle
 *                     (reconnect, IP acquisition) is driven by the event
 *                     handler so no task stack is wasted at idle.
 *
 *   ntp_sync_task   — Blocks indefinitely on WIFI_CONNECTED_BIT.  Once
 *                     WiFi is up it syncs SNTP, tracks the timestamp of
 *                     the last successful sync, and re-syncs whenever the
 *                     link reconnects or the sync is older than
 *                     NTP_RESYNC_INTERVAL_S (default 1 hour).  Falls back
 *                     to a 60-second retry on failure.
 *
 * ## EventGroup bits
 *
 *   WIFI_CONNECTED_BIT — Set when an IP is obtained; cleared on disconnect.
 *   WIFI_RECONNECT_BIT — Pulsed on every successful connect (including the
 *                        first one) so ntp_sync_task knows to re-sync even
 *                        when the clock was already synced recently.
 *                        Cleared by ntp_sync_task before the sync starts so
 *                        a mid-sync reconnect is never silently discarded.
 *
 * ## Thread safety
 *
 *   s_ntp_synced  — declared volatile; written only from ntp_sync_task and
 *                   the WiFi event handler (both are single-writer paths).
 *   EventGroup    — FreeRTOS EventGroup operations are atomically safe from
 *                   any task context.
 */

#include "net.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

/* ── Constants ─────────────────────────────────────────────────────────────*/

/** EventGroup bit: WiFi has an IP address. */
#define WIFI_CONNECTED_BIT BIT0

/**
 * EventGroup bit: pulsed on each successful WiFi connect.
 * Tells ntp_sync_task to trigger an immediate re-sync.
 */
#define WIFI_RECONNECT_BIT BIT1

/** Re-sync NTP after this many seconds since the last successful sync. */
#define NTP_RESYNC_INTERVAL_S (3600)

/** Seconds to wait for a single SNTP exchange before giving up. */
#define NTP_SYNC_TIMEOUT_S (30)

/** FreeRTOS stack (bytes) for the one-shot WiFi init task. */
#define WIFI_TASK_STACK (6144)

/** FreeRTOS stack (bytes) for the long-lived NTP sync task. */
#define NTP_TASK_STACK (4096)

/* ── Module state ───────────────────────────────────────────────────────── */

static const char *TAG = "net";

static char s_ssid[32];
static char s_password[64];
static char s_tz_str[32];

/** EventGroup shared between the WiFi event handler and ntp_sync_task. */
static EventGroupHandle_t s_net_events;

/**
 * True once ntp_sync_task has recorded a successful SNTP exchange.
 * Cleared by the WiFi event handler on disconnect so callers know the
 * displayed time may be stale.
 */
static volatile bool s_ntp_synced = false;

/** Wall-clock time of the last successful NTP sync (0 = never synced). */
static time_t s_last_ntp_sync = 0;

/** Guards against double-initialisation (e.g. soft reset). */
static bool s_initialized = false;

/* ── Time helpers ───────────────────────────────────────────────────────── */

void wait_for_sec(int seconds)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    /* ms already elapsed in the current `seconds`-wide window */
    uint32_t ms_elapsed = (uint32_t)(tv.tv_usec / 1000);
    uint32_t ms_to_next = (uint32_t)(seconds * 1000) - ms_elapsed;
    vTaskDelay(pdMS_TO_TICKS(ms_to_next));
}

static char s_time_buf[12]; /* "HH:MM:SS AM\0" */
static char s_date_buf[12]; /* "DD MMM YYYY\0" */

const char *get_ntp_time_string(void)
{
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    strftime(s_time_buf, sizeof(s_time_buf), "%I:%M:%S %p", &t);
    return s_time_buf;
}

const char *get_ntp_date_string(void)
{
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    strftime(s_date_buf, sizeof(s_date_buf), "%d %b %Y", &t);
    return s_date_buf;
}

/* ── Status queries ─────────────────────────────────────────────────────── */

bool is_wifi_connected(void)
{
    if (!s_net_events)
        return false;
    return (xEventGroupGetBits(s_net_events) & WIFI_CONNECTED_BIT) != 0;
}

bool is_ntp_synced(void)
{
    return s_ntp_synced;
}

/* ── WiFi event handler ─────────────────────────────────────────────────── */

/**
 * Runs in the ESP-IDF system-event task — must not block.
 *
 * WIFI_EVENT_STA_START        → initiate the first connection attempt.
 * WIFI_EVENT_STA_DISCONNECTED → clear WiFi status, invalidate NTP flag,
 *                               and immediately request a reconnect.
 *                               No retry limit: the driver keeps trying
 *                               until the AP is reachable again.
 * IP_EVENT_STA_GOT_IP         → record the address, signal both
 *                               WIFI_CONNECTED_BIT and WIFI_RECONNECT_BIT.
 */
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(s_net_events, WIFI_CONNECTED_BIT);
        s_ntp_synced = false; /* time may be stale — force re-sync on reconnect */
        ESP_LOGW(TAG, "WiFi disconnected — reconnecting...");
        esp_wifi_connect(); /* always retry; no give-up counter */
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "WiFi connected  IP:" IPSTR, IP2STR(&ev->ip_info.ip));
        /* Set both bits: CONNECTED for status queries, RECONNECT to wake NTP. */
        xEventGroupSetBits(s_net_events, WIFI_CONNECTED_BIT | WIFI_RECONNECT_BIT);
    }
}

/* ── WiFi init task (one-shot) ──────────────────────────────────────────── */

/**
 * Initialises NVS, the TCP/IP stack, and the WiFi driver, then exits.
 * All subsequent WiFi activity is driven by wifi_event_handler via the
 * default event loop — no task stack is consumed at idle.
 */
static void wifi_init_task(void *arg)
{
    /* NVS is required by the WiFi driver. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());

    /* esp_event_loop_create_default() returns ESP_ERR_INVALID_STATE if the
     * loop already exists (e.g. after a soft reset) — treat that as OK. */
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        ESP_ERROR_CHECK(err);

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Register the shared handler for all WiFi events and the IP-got event. */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_cfg = {0};
    strncpy((char *)wifi_cfg.sta.ssid, s_ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, s_password, sizeof(wifi_cfg.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start()); /* triggers WIFI_EVENT_STA_START → connect */

    ESP_LOGI(TAG, "WiFi started, connecting to \"%s\"...", s_ssid);
    vTaskDelete(NULL); /* wifi_event_handler drives everything from here */
}

/* ── NTP sync task (permanent) ──────────────────────────────────────────── */

/**
 * Waits for WiFi (WIFI_CONNECTED_BIT) then decides whether to (re-)sync:
 *   - On the first connect after boot (s_last_ntp_sync == 0).
 *   - On every subsequent reconnect (WIFI_RECONNECT_BIT is set).
 *   - When the last sync is older than NTP_RESYNC_INTERVAL_S (1 hour).
 *
 * On success, sets s_ntp_synced and records s_last_ntp_sync.
 * On failure (WiFi lost mid-sync or SNTP timeout), retries in 60 s.
 */
static void ntp_sync_task(void *arg)
{
    /* Apply timezone before the first time string is ever formatted. */
    if (s_tz_str[0])
    {
        setenv("TZ", s_tz_str, 1);
        tzset();
        ESP_LOGI(TAG, "Timezone set to %s", s_tz_str);
    }

    /* Initialise the SNTP driver once.  esp_sntp_restart() is used later to
     * trigger on-demand re-syncs without reinitialising the driver. */
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_setservername(2, "time.cloudflare.com");
    esp_sntp_init();

    while (1)
    {
        /* Block until WiFi has an IP — no CPU cost while disconnected. */
        xEventGroupWaitBits(s_net_events, WIFI_CONNECTED_BIT,
                            pdFALSE, pdTRUE, portMAX_DELAY);

        bool reconnected = (xEventGroupGetBits(s_net_events) & WIFI_RECONNECT_BIT) != 0;
        time_t now = time(NULL);
        bool stale = (s_last_ntp_sync == 0) ||
                     ((now - s_last_ntp_sync) > NTP_RESYNC_INTERVAL_S);

        if (reconnected || stale)
        {
            /*
             * Clear WIFI_RECONNECT_BIT *before* starting the sync so that
             * a reconnect occurring mid-sync leaves the bit set for the
             * next loop iteration — we never silently lose a reconnect event.
             */
            xEventGroupClearBits(s_net_events, WIFI_RECONNECT_BIT);
            s_ntp_synced = false;

            ESP_LOGI(TAG, "Starting NTP sync (%s)...",
                     reconnected ? "reconnect" : "clock stale");

            /* Trigger a fresh SNTP exchange (status → RESET → COMPLETED). */
            esp_sntp_restart();

            bool synced = false;
            for (int i = 0; i < NTP_SYNC_TIMEOUT_S; i++)
            {
                /* Abort early if WiFi drops during the sync. */
                if (!(xEventGroupGetBits(s_net_events) & WIFI_CONNECTED_BIT))
                {
                    ESP_LOGW(TAG, "WiFi lost during NTP sync — aborting");
                    break;
                }
                if (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED)
                {
                    synced = true;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(1000));
            }

            if (synced)
            {
                s_ntp_synced = true;
                s_last_ntp_sync = time(NULL);
                ESP_LOGI(TAG, "NTP synced — %s %s",
                         get_ntp_date_string(), get_ntp_time_string());
            }
            else
            {
                ESP_LOGW(TAG, "NTP sync failed — will retry in 60 s");
            }
        }

        /* Rate-limit: check again in 60 s (also serves as retry delay). */
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

/* ── Public API ─────────────────────────────────────────────────────────── */

esp_err_t net_start(const char *ssid, const char *password, const char *tz_str)
{
    if (s_initialized)
    {
        ESP_LOGW(TAG, "net_start() called more than once — ignoring");
        return ESP_ERR_INVALID_STATE;
    }
    if (!ssid || !password)
        return ESP_ERR_INVALID_ARG;

    strncpy(s_ssid, ssid, sizeof(s_ssid) - 1);
    strncpy(s_password, password, sizeof(s_password) - 1);
    if (tz_str)
        strncpy(s_tz_str, tz_str, sizeof(s_tz_str) - 1);

    s_net_events = xEventGroupCreate();
    if (!s_net_events)
        return ESP_ERR_NO_MEM;

    if (xTaskCreate(wifi_init_task, "wifi_init", WIFI_TASK_STACK,
                    NULL, 5, NULL) != pdPASS)
        return ESP_FAIL;

    if (xTaskCreate(ntp_sync_task, "ntp_sync", NTP_TASK_STACK,
                    NULL, 4, NULL) != pdPASS)
        return ESP_FAIL;

    s_initialized = true;
    return ESP_OK;
}
