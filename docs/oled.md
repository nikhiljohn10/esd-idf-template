# oled

SSD1306 128x32 OLED display driver with text rendering, bitmap drawing, animated WiFi icon, and auto-dim.

**Header:** `lib/oled/oled.h`  
**Depends on:** `k0i05/esp_ssd1306` (declared in `platformio.ini`)

---

## Hardware

| Signal       | GPIO        |
| ------------ | ----------- |
| I2C SDA      | 21          |
| I2C SCL      | 22          |
| Display size | 128 × 32 px |

---

## API

### `oled_config_t`

```c
typedef struct {
    i2c_port_num_t i2c_port; // I2C peripheral, e.g. I2C_NUM_0
    int sda_pin;             // GPIO for SDA
    int scl_pin;             // GPIO for SCL
} oled_config_t;
```

### `oled_handle_t setup_screen(const oled_config_t *config)`

Obtains the shared I2C bus and initialises the SSD1306 display. Returns an opaque handle required by all other functions.

```c
oled_handle_t oled = setup_screen(&(oled_config_t){
    .i2c_port = I2C_NUM_0,
    .sda_pin  = 21,
    .scl_pin  = 22,
});
```

Because `setup_screen()` uses the `i2c_bus` shared bus manager, it is safe to call even if `bme280_init()` has already been called on the same port and pins — the bus will be reused rather than re-created.

---

### `void oled_clear(oled_handle_t handle)`

Clears the entire display framebuffer to black.

```c
oled_clear(oled);
```

---

### `void oled_show(oled_handle_t handle)`

Flushes the current framebuffer to the display. Call this after drawing operations to make them visible.

```c
oled_show(oled);
```

---

### `void set_text(oled_handle_t handle, const char *text, int xpos, int ypos)`

Renders a string using the built-in 8×8 Latin font at pixel position `(xpos, ypos)`.

- `xpos` / `ypos` are in pixels (not characters).
- Characters that exceed the display width are clipped.

```c
set_text(oled, "Hello ESP32", 0, 0);
```

---

### `void set_text_scroll(oled_handle_t handle, const char *text, int ypos, int speed)`

Renders a scrolling ticker on a single pixel row. Uses a three-state machine:

1. **Hold left** — display starts at `x=0`, holds for ~1 s.
2. **Scroll right** — text advances pixel-by-pixel until the end is reached.
3. **Hold right** — holds for ~1 s, then resets to state 1.

If the text fits entirely within 128 pixels (≤ 16 characters in the built-in 8×8 font), it is rendered statically at `x=0` and the function returns immediately without scrolling.

- `speed` — pixels advanced per call (1 = smooth, higher = faster)
- Call once per render-loop iteration (e.g. every 100 ms)

```c
set_text_scroll(oled, "Temperature: 23.5 C  Humidity: 65%  Pressure: 1013 hPa", 24, 1);
```

---

### `void draw_bitmap_8x8(oled_handle_t handle, const uint8_t *bitmap, int xpos, int ypos)`

Draws a single 8×8 monochrome bitmap tile.

```c
draw_bitmap_8x8(oled, my_icon, 112, 0); // top-right corner
```

---

### `void draw_wifi_icon(oled_handle_t handle, wifi_icon_state_t state, int xpos, int ypos)`

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

### `void oled_render_home(oled_handle_t handle, const char *title, wifi_icon_state_t wifi_state, const char *status_text)`

Renders the standard home screen layout:

```
[title                    ] [wifi icon]
[status_text                         ]
```

- `title` — top-left label (e.g. `"LED Lamp!"`)
- `wifi_state` — drives the WiFi icon in the top-right corner
- `status_text` — bottom line (IP address, error message, etc.)

```c
oled_render_home(oled, "LED Lamp!", WIFI_ICON_CONNECTED, "192.168.1.50");
```

---

### `esp_err_t brightness_control(oled_handle_t handle, int value, int timeout_seconds)`

Auto-dims the display contrast after `timeout_seconds` of no change in `value`.

- While `value` changes between calls: contrast = `0xFF` (full brightness)
- After `timeout_seconds` with no change: contrast = `0x01` (dim)

Call this once per render loop, passing the current sensor/input value.

```c
brightness_control(oled, led_brightness, 5); // dim after 5 s of no ADC change
```

Returns `ESP_OK`.

---

## Constants (`oled_const.h`)

### `UNIT_TEMP_C`

```c
#define UNIT_TEMP_C '\xB0'
```

Degree symbol (`°`) in the ISO-8859-1 encoding used by `font_latin_8x8_tr`. Use this constant rather than a raw byte to keep source files ASCII-clean.

```c
char buf[32];
snprintf(buf, sizeof(buf), "Temp: %.1f %cC", temp, UNIT_TEMP_C);
set_text(oled, buf, 0, 0);
```

---

## Drawing primitives (`oled_anim.h`)

Low-level pixel drawing wrappers built on top of the `esp_ssd1306` raw pixel API. All coordinates are in pixels; origin is the top-left corner.

| Function | Description |
|---|---|
| `oled_set_pixel(handle, x, y, on)` | Set or clear a single pixel |
| `oled_draw_line(handle, x0, y0, x1, y1)` | Bresenham line |
| `oled_draw_rect(handle, x, y, w, h)` | Hollow rectangle |
| `oled_fill_rect(handle, x, y, w, h)` | Filled rectangle |
| `oled_draw_circle(handle, cx, cy, r)` | Hollow circle (midpoint algorithm) |
| `oled_fill_circle(handle, cx, cy, r)` | Filled circle |
| `oled_draw_bitmap(handle, bitmap, x, y, w, h)` | Arbitrary-size 1-bpp bitmap |
| `oled_flush(handle)` | Push pixel buffer to display |

### Animation helpers

Predefined full-screen animation patterns. Call once per frame inside a loop:

| Function | Pattern |
|---|---|
| `oled_anim_all_on(handle)` | Fill every pixel white |
| `oled_anim_checkerboard(handle, frame)` | Alternating checkerboard, inverted per frame |
| `oled_anim_vertical_stripes(handle, frame)` | Moving vertical stripe pattern |
| `oled_anim_horizontal_stripes(handle, frame)` | Moving horizontal stripe pattern |
| `oled_anim_concentric_rects(handle, frame)` | Expanding concentric rectangles |

---

## Full render loop example

```c
#include "oled.h"
#include "utils.h"

void screen_task(void *arg) {
    oled_handle_t oled = setup_screen(&(oled_config_t){
        .i2c_port = I2C_NUM_0,
        .sda_pin  = 21,
        .scl_pin  = 22,
    });

    while (1) {
        brightness_control(oled, 0, 5);
        oled_render_home(oled, "My Device", WIFI_ICON_CONNECTED, "192.168.1.50");
        delay(25);
    }
}
```

---

## Notes

- I2C pins (SDA=21, SCL=22) are the default wiring; configure them via `oled_config_t` on each call to `setup_screen()`.
- `oled_handle_t` is an opaque pointer (`struct oled_context_t *`). Do not cast it to an `ssd1306_handle_t`.
- The display buffer is not double-buffered; partial redraws may cause brief flicker on fast updates.
- `draw_wifi_icon` with `WIFI_ICON_CONNECTING` uses an internal static frame counter — it is not thread-safe if called from multiple tasks.
- `set_text_scroll` uses `esp_timer_get_time()` for 1-second hold timing; no FreeRTOS dependencies.
