#ifndef NET_H
#define NET_H

/**
 * @file net.h
 * @brief Resilient WiFi + NTP network library for ESP32 (ESP-IDF 5.x).
 *
 * ## Architecture
 *
 * Calling net_start() launches two background FreeRTOS tasks:
 *
 *   wifi_init_task  — Initialises WiFi hardware once, then exits.
 *                     A shared event handler reconnects automatically
 *                     whenever the link drops (unlimited retries, no
 *                     give-up timer).
 *
 *   ntp_sync_task   — Blocks on the WiFi EventGroup, syncs SNTP on
 *                     every reconnect and at most once per hour,
 *                     then sleeps.  Never busy-waits.
 *
 * An EventGroup (WIFI_CONNECTED_BIT / WIFI_RECONNECT_BIT) carries WiFi
 * state between the event handler and the NTP task without polling.
 *
 * ## Typical usage
 *
 * @code
 *   net_start("MySSID", "s3cr3t", "IST-5:30");
 *
 *   while (1) {
 *       if (is_ntp_synced())
 *           printf("%s  %s\n", get_ntp_date_string(), get_ntp_time_string());
 *       wait_for_sec(1);
 *   }
 * @endcode
 */

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialise WiFi and NTP as background tasks.
 *
 * Stores credentials, creates a FreeRTOS EventGroup, then launches
 * wifi_init_task and ntp_sync_task.  May be called only once; every
 * subsequent call returns ESP_ERR_INVALID_STATE (soft-reset safe).
 *
 * @param ssid      WiFi SSID.
 * @param password  WiFi password (WPA2 / open network).
 * @param tz_str    POSIX timezone string applied to the local clock,
 *                  e.g. "IST-5:30", "CET-1CEST,M3.5.0,M10.5.0/3",
 *                  or "UTC0".  Pass NULL to leave the timezone unchanged.
 *
 * @return  ESP_OK              Tasks started successfully.
 *          ESP_ERR_INVALID_ARG ssid or password is NULL.
 *          ESP_ERR_INVALID_STATE Already initialised.
 *          ESP_ERR_NO_MEM      EventGroup allocation failed.
 *          ESP_FAIL            Task creation failed.
 */
esp_err_t net_start(const char *ssid, const char *password, const char *tz_str);

/**
 * @brief Return true while WiFi is associated and has an IP address.
 *
 * Reads the EventGroup bit set by the WiFi event handler.
 * Thread-safe; callable from any task.
 */
bool is_wifi_connected(void);

/**
 * @brief Return true once SNTP has completed at least one successful sync.
 *
 * Cleared automatically on WiFi disconnect so callers know the
 * displayed time may be stale.  Reads a volatile flag — thread-safe.
 */
bool is_ntp_synced(void);

/**
 * @brief Current local time formatted as "HH:MM:SS AM" / "HH:MM:SS PM".
 *
 * Uses a 12-byte static buffer — not re-entrant.
 * Only meaningful after is_ntp_synced() returns true.
 *
 * @return Pointer to the internal buffer (valid until the next call).
 */
const char *get_ntp_time_string(void);

/**
 * @brief Current local date formatted as "YYYY-MM-DD".
 *
 * Uses an 11-byte static buffer — not re-entrant.
 * Only meaningful after is_ntp_synced() returns true.
 *
 * @return Pointer to the internal buffer (valid until the next call).
 */
const char *get_ntp_date_string(void);

/**
 * @brief Sleep until the next `seconds`-aligned wall-clock boundary.
 *
 * Unlike vTaskDelay(pdMS_TO_TICKS(1000)), this compensates for the
 * sub-second offset at call time so that repeated calls stay
 * phase-locked to real seconds with no cumulative drift.
 *
 * Example: wait_for_sec(1) wakes at the start of the next whole second.
 *
 * @param seconds  Boundary interval in seconds (typically 1).
 */
void wait_for_sec(int seconds);

#endif /* NET_H */
