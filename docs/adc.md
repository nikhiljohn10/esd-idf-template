# adc

ADC oneshot sampling with configurable moving average.

**Header:** `lib/adc/adc.h`

---

## Overview

Wraps ESP-IDF's `esp_adc/adc_oneshot` API. Configures ADC Unit 1 at 12 dB attenuation (0 – ~3.1 V input range) and returns an averaged reading across 10 samples.

---

## API

### `adc_oneshot_unit_handle_t setup_light_sensor(adc_channel_t channel)`

Initialises ADC Unit 1 and configures the given channel.

Returns a handle that must be passed to `get_sample_value()`.

```c
#include "esp_adc/adc_oneshot.h"
#include "adc.h"

adc_oneshot_unit_handle_t adc = setup_light_sensor(ADC_CHANNEL_7);
```

**Configuration applied:**

| Setting | Value |
|---------|-------|
| ADC unit | `ADC_UNIT_1` |
| Bit width | `ADC_BITWIDTH_DEFAULT` (12-bit on ESP32) |
| Attenuation | `ADC_ATTEN_DB_12` (~0–3.1 V) |

---

### `int get_sample_value(adc_oneshot_unit_handle_t adc_handle, adc_channel_t channel, int *out_raw)`

Takes 10 samples with 10 ms between each (total ~100 ms), writes the last raw reading into `*out_raw`, and returns the integer average.

```c
int raw;
int avg = get_sample_value(adc, ADC_CHANNEL_7, &raw);
printf("avg=%d  raw=%d\n", avg, raw);
```

The returned value is in the range `0 – 4095` (12-bit).

---

## Example — ambient-light-controlled LED brightness

```c
#include "esp_adc/adc_oneshot.h"
#include "adc.h"
#include "led.h"
#include "utils.h"

void app_main(void) {
    adc_channel_t channel = ADC_CHANNEL_7;
    adc_oneshot_unit_handle_t adc = setup_light_sensor(channel);
    ledc_channel_t lamp = setup_dimmable_led(GPIO_NUM_27, LEDC_CHANNEL_0);

    int raw;
    while (1) {
        int lux = get_sample_value(adc, channel, &raw);
        int brightness = MAX(0, 255 - ((lux * 255) / 3000));
        dimmable_led_out(lamp, brightness);
    }
}
```

---

## Notes

- `setup_light_sensor()` calls `ESP_ERROR_CHECK` internally — any hardware error will abort the program with a panic.
- The 10 ms per-sample delay is a blocking `vTaskDelay` — run this in a dedicated FreeRTOS task to avoid starving other tasks.
- Raw ADC readings on ESP32 are non-linear; apply `esp_adc_cal` calibration for accurate voltage measurements.
