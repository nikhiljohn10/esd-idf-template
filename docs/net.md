# net

WiFi STA connection and SNTP time synchronisation.

**Header:** `lib/net/net.h`

---

## API

### `esp_err_t setup_wifi(const char *ssid, const char *password)`

Initialises NVS, the TCP/IP stack, and the WiFi driver in station mode. Attempts to connect to the given network and blocks until an IP address is assigned or all retries are exhausted.

Returns `ESP_OK` on success, or an error code on failure.

```c
#include "net.h"

esp_err_t err = setup_wifi("my-network", "my-password");
if (err != ESP_OK) {
    printf("WiFi failed: %s\n", esp_err_to_name(err));
}
```

**Behaviour:**

- Registers event handlers for `WIFI_EVENT` (connect / disconnect) and `IP_EVENT` (got IP).
- Retries up to 5 times before giving up.
- Uses an `EventGroup` to block the caller — the function returns as soon as an IP is assigned or all retries fail.

---

### `esp_err_t setup_network(const char *tz_str)`

Synchronises system time using SNTP against `pool.ntp.org`. Blocks until the clock is set, polling every second up to a 10-second timeout.

Must be called **after** `setup_wifi()` succeeds.

| Return value          | Meaning                                              |
| --------------------- | ---------------------------------------------------- |
| `ESP_OK`              | Time synced successfully                             |
| `ESP_FAIL`            | NTP sync timed out after 10 s                        |
| `ESP_ERR_INVALID_ARG` | `tz_str` is non-NULL but not a valid POSIX TZ string |

**`tz_str`** — optional POSIX timezone string applied after sync. Pass `NULL` to skip timezone configuration.

```c
// No timezone config
setup_network(NULL);

// India Standard Time
setup_network("IST-5:30");

// US Eastern with DST
setup_network("EST5EDT,M3.2.0,M11.1.0");
```

POSIX TZ format: `StdName±offset[DstName[offset][,rule]]`  
Examples: `"UTC0"`, `"IST-5:30"`, `"CET-1CEST,M3.5.0,M10.5.0/3"`

If an invalid string is passed (e.g. a Windows-style name like `"India Standard Time"`), the function logs an error and returns `ESP_ERR_INVALID_ARG` without initialising SNTP.

**Why this is needed:** ESP32 has no battery-backed RTC. The system clock starts at the Unix epoch (1970). TLS handshakes will fail with certificate errors until the time is corrected via SNTP.

---

## Example

```c
#include "net.h"
#include "config.h" // WIFI_SSID, WIFI_PASSWORD
#include <stdio.h>

void app_main(void) {
    if (setup_wifi(WIFI_SSID, WIFI_PASSWORD) != ESP_OK) {
        printf("Failed to connect\n");
        return;
    }
    printf("Connected\n");

    setup_network("UTC0"); // sync time for TLS (pass NULL to skip timezone)
    printf("Time synced\n");

    // Safe to make HTTPS requests now
}
```

---

## Notes

- `setup_wifi()` stores SSID and password in static buffers; it is safe to pass stack strings.
- Calling `setup_wifi()` again without a reboot is not supported — initialise once at startup.
- If your application does not need TLS, you can skip `setup_network()`.
- WiFi credentials should be defined in `include/config.h` (gitignored). See `include/config.h.example`.
