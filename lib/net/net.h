#ifndef NET_H
#define NET_H

#include "esp_err.h"

/**
 * Initialises NVS, the TCP/IP stack, and WiFi in station mode.
 * Blocks until an IP address is obtained or all retries are exhausted.
 */
esp_err_t setup_wifi(const char *ssid, const char *password);

/**
 * Synchronises system time via SNTP (pool.ntp.org).
 * Needed for TLS certificate validation. Blocks up to ~10 s.
 */
esp_err_t setup_network(void);

#endif // NET_H
