# net

WiFi STA connection and SNTP time synchronisation.

**Header:** `lib/net/net.h`

---

## Overview

`net_start()` is non-blocking. It spawns two background FreeRTOS tasks:

- **`wifi_init_task`** — Initialises the TCP/IP stack and WiFi driver, connects to the network, and retries on disconnect.
- **`ntp_sync_task`** — Waits for an IP address, then syncs via SNTP and sets the configured timezone.

`app_main` can continue immediately after `net_start()` returns. Use `is_wifi_connected()` and `is_ntp_synced()` to poll status.

---

## API

### `esp_err_t net_start(const char *ssid, const char *password, const char *tz_str)`

Launches WiFi + NTP background tasks. Returns immediately.

- `ssid` / `password` — WiFi credentials.
- `tz_str` — POSIX timezone string applied after NTP sync. Pass `NULL` to use UTC.

Returns `ESP_OK` if the tasks were created successfully.

```c
#include "net.h"
#include "config.h" // WIFI_SSID, WIFI_PASSWORD

void app_main(void) {
    net_start(WIFI_SSID, WIFI_PASSWORD, "IST-5:30");
    // returns immediately — WiFi connects in the background
}
```

**`tz_str` examples:**

| Region | String |
|---|---|
| UTC | `"UTC0"` |
| India Standard Time | `"IST-5:30"` |
| US Eastern with DST | `"EST5EDT,M3.2.0,M11.1.0"` |
| Central Europe with DST | `"CET-1CEST,M3.5.0,M10.5.0/3"` |

POSIX TZ format: `StdName±offset[DstName[offset][,rule]]`

---

### `bool is_wifi_connected(void)`

Returns `true` if the station has an IP address. Thread-safe (reads an EventGroup bit).

```c
if (is_wifi_connected()) {
    printf("IP assigned\n");
}
```

---

### `bool is_ntp_synced(void)`

Returns `true` once the SNTP sync has completed and the system clock is valid. Thread-safe (reads a `volatile` flag).

```c
if (is_ntp_synced()) {
    printf("Clock is accurate\n");
}
```

---

### `const char *get_ntp_time_string(void)`

Returns the current wall-clock time as a 12-hour `"HH:MM:SS AM/PM"` string.

- Backed by a 12-byte static buffer — not re-entrant.
- Returns `"--:--:-- --"` if NTP has not yet synced.

```c
printf("Time: %s\n", get_ntp_time_string());
```

---

### `const char *get_ntp_date_string(void)`

Returns the current date as an `"YYYY-MM-DD"` string.

- Backed by an 11-byte static buffer — not re-entrant.
- Returns `"----/--/--"` if NTP has not yet synced.

```c
printf("Date: %s\n", get_ntp_date_string());
```

---

### `void wait_for_sec(int seconds)`

Phase-locked sleep that wakes at the next whole-second boundary on the wall clock, then waits an additional `seconds - 1` full seconds. This lets a timed loop stay aligned to the real-time clock rather than drifting.

```c
while (1) {
    printf("%s\n", get_ntp_time_string());
    wait_for_sec(1); // wakes exactly at the next whole second
}
```

---

## Example

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "net.h"
#include "config.h" // WIFI_SSID, WIFI_PASSWORD
#include "utils.h"

void app_main(void) {
    net_start(WIFI_SSID, WIFI_PASSWORD, "UTC0");

    // Other initialisation can happen here while WiFi connects...

    while (!is_ntp_synced()) {
        delay(500);
    }
    printf("Date: %s  Time: %s\n", get_ntp_date_string(), get_ntp_time_string());

    while (1) {
        wait_for_sec(1);
        printf("%s\n", get_ntp_time_string());
    }
}
```

---

## Notes

- Credentials are stored in static buffers inside `net_start()`; stack strings are safe to pass.
- Call `net_start()` only once per boot. Reinitialising WiFi without a reboot is not supported.
- WiFi credentials should be defined in `include/config.h` (gitignored). See `include/config.h.example`.
- If your application does not need accurate time, you can ignore `is_ntp_synced()` and the time-string functions.
- The string buffers returned by `get_ntp_time_string()` and `get_ntp_date_string()` are overwritten on each call — copy the result if you need to hold it across calls.
