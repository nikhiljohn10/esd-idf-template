# water_level

Driver for low-cost **analog water-level / depth sensor modules** (resistive trace boards such as the Funduino "water sensor", T1592, etc.) on any ADC1 channel.

**Header:** `lib/water_level/water_level.h`

---

## Overview

These sensors expose a series of interleaved conductive traces. Submersion in water bridges the traces, lowering the module's resistance. A built-in voltage divider produces an analog signal — typically **0 V (dry) → ~2.5 V (fully submerged)** when powered from 3.3 V.

This driver:

- Configures **ADC1** for the chosen channel at 12 dB attenuation (~0–3.1 V).
- Averages a configurable number of samples per read.
- Linearly maps the averaged raw value into a calibrated **0–100 %** depth using a `raw_dry` / `raw_full` calibration pair.

---

## API

### `water_level_config_t`

```c
typedef struct {
    adc_channel_t channel;   // ADC1 channel (e.g. ADC_CHANNEL_6 = GPIO34)
    int raw_dry;             // Raw ADC reading when fully out of water
    int raw_full;            // Raw ADC reading when fully submerged
    int sample_count;        // Samples to average per read (e.g. 10)
    int sample_delay_ms;     // Delay between samples in ms (e.g. 10)
} water_level_config_t;
```

### `water_level_reading_t`

```c
typedef struct {
    int raw;        // Averaged 0–4095 ADC value
    int percent;    // 0–100 % depth (clamped)
} water_level_reading_t;
```

### `esp_err_t water_level_init(const water_level_config_t *config, adc_oneshot_unit_handle_t *handle)`

Initialises ADC1 and configures the requested channel. The returned handle must be passed to `water_level_read()`.

### `esp_err_t water_level_read(adc_oneshot_unit_handle_t handle, const water_level_config_t *config, water_level_reading_t *out)`

Takes `sample_count` samples (with `sample_delay_ms` between each), averages them, and maps the result onto 0–100 %.

---

## Wiring

| Sensor pin | ESP32 pin (example) |
| ---------- | ------------------- |
| `+` (VCC)  | 3V3                 |
| `-` (GND)  | GND                 |
| `S` (Sig)  | GPIO 34 (ADC1_CH6)  |

> Use any ADC1-capable GPIO. ADC2 is not supported because it conflicts with WiFi.

> Powering the sensor only while reading (via a GPIO + transistor) prevents galvanic corrosion of the traces. For continuous monitoring, consider capacitive sensors instead.

---

## Calibration

Both `raw_dry` and `raw_full` are board-specific. Calibrate once:

1. Flash the example with placeholder values (e.g. 100 / 3000).
2. Watch the serial output with the sensor **dry** — record the stable raw value as `raw_dry`.
3. Submerge the sensor up to its maximum useful depth — record that raw value as `raw_full`.
4. Update the constants and re-flash.

---

## Example

```c
#include "esp_adc/adc_oneshot.h"
#include "water_level.h"
#include "utils.h"

void app_main(void) {
    water_level_config_t cfg = {
        .channel         = ADC_CHANNEL_6,   // GPIO34 on ESP32
        .raw_dry         = 100,
        .raw_full        = 2800,
        .sample_count    = 10,
        .sample_delay_ms = 10,
    };

    adc_oneshot_unit_handle_t adc;
    ESP_ERROR_CHECK(water_level_init(&cfg, &adc));

    while (1) {
        water_level_reading_t r;
        if (water_level_read(adc, &cfg, &r) == ESP_OK) {
            printf("raw=%d  level=%d%%\n", r.raw, r.percent);
        }
        delay(500);
    }
}
```

See [`examples/water_level_tank/`](../examples/water_level_tank/) for an end-to-end demo that visualises the level on the OLED.

---

## Notes

- Each call to `water_level_read()` blocks for roughly `sample_count * sample_delay_ms` milliseconds.
- The driver creates its own ADC1 oneshot unit. If your project already creates one (e.g. via the `adc` library), share a single unit by configuring the channel directly with `adc_oneshot_config_channel()` instead of calling `water_level_init()`.
