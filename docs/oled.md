# oled

SSD1306 128x32 OLED display driver with text rendering, bitmap drawing, animated WiFi icon, and auto-dim.

**Header:** `lib/oled/oled.h`  
**Depends on:** `k0i05/esp_ssd1306` (declared in `platformio.ini`)

---

## Hardware

| Signal | GPIO |
|--------|------|
| I2C SDA | 21 |
| I2C SCL | 22 |
| Display size | 128 × 32 px |

---

## API

### `ssd1306_handle_t setup_screen(void)`

Initialises the I2C bus and SSD1306 display. Returns a handle required by all other functions.

```c
ssd1306_handle_t oled = setup_screen();
```

---

### `void set_latin_text(ssd1306_handle_t handle, const char *text, int xpos, int ypos)`

Renders a string using the built-in 8×8 Latin font at pixel position `(xpos, ypos)`.

- `xpos` / `ypos` are in pixels (not characters).
- Characters that exceed the display width are clipped.

```c
set_latin_text(oled, "Hello ESP32", 0, 0);
```

---

### `void draw_bitmap_8x8(ssd1306_handle_t handle, const uint8_t *bitmap, int xpos, int ypos)`

Draws a single 8×8 monochrome bitmap tile.

```c
draw_bitmap_8x8(oled, my_icon, 112, 0); // top-right corner
```

---

### `void draw_wifi_icon(ssd1306_handle_t handle, wifi_icon_state_t state, int xpos, int ypos)`

Draws a WiFi status icon based on the current state.

```c
typedef enum {
    WIFI_ICON_CONNECTING = 0,  // animated (4-frame cycle)
    WIFI_ICON_CONNECTED,       // static full-signal icon
    WIFI_ICON_FAILED,          // X pattern
} wifi_icon_state_t;
```

The animation frame advances on each call (use in a loop calling every ~25 ms).

```c
draw_wifi_icon(oled, WIFI_ICON_CONNECTING, 108, 0);
```

---

### `void oled_render_home(ssd1306_handle_t handle, const char *title, wifi_icon_state_t wifi_state, const char *status_text, int led_brightness)`

Renders the standard home screen layout:

```
[title                    ] [wifi icon]
[brightness bar                      ]
[status_text                         ]
```

- `title` — top-left label (e.g. `"LED Lamp!"`)
- `wifi_state` — drives the WiFi icon in the top-right corner
- `status_text` — bottom line (IP address, error message, etc.)
- `led_brightness` — `0–255`; drawn as a proportional bar on the second line

```c
oled_render_home(oled, "LED Lamp!", WIFI_ICON_CONNECTED, "1.2.3.4", 200);
```

---

### `esp_err_t brightness_control(ssd1306_handle_t handle, int value, int timeout_seconds)`

Auto-dims the display contrast after `timeout_seconds` of no change in `value`.

- While `value` changes between calls: contrast = `0xFF` (full brightness)
- After `timeout_seconds` with no change: contrast = `0x01` (dim)

Call this once per render loop, passing the current sensor/input value.

```c
brightness_control(oled, led_brightness, 5); // dim after 5 s of no ADC change
```

Returns `ESP_OK`.

---

## Full render loop example

```c
#include "ssd1306.h"
#include "oled.h"
#include "utils.h"

void screen_task(void *arg) {
    ssd1306_handle_t oled = setup_screen();
    int brightness = 128;

    while (1) {
        brightness_control(oled, brightness, 5);
        oled_render_home(oled, "My Device", WIFI_ICON_CONNECTED, "192.168.1.50", brightness);
        delay(25);
    }
}
```

---

## Notes

- I2C pins (SDA=21, SCL=22) are hardcoded in `oled.c`. Change the `#define` constants there if your board differs.
- The display buffer is not double-buffered; partial redraws may cause brief flicker on fast updates.
- `draw_wifi_icon` with `WIFI_ICON_CONNECTING` uses an internal static frame counter — it is not thread-safe if called from multiple tasks.
