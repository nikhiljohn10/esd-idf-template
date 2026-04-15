# led

GPIO LED control and LEDC PWM dimming.

**Header:** `lib/led/led.h`

---

## API

### `int setup_led(int gpio_pin)`

Configures a GPIO pin as a push-pull output. Returns the pin number (pass back to `led_out()`).

```c
int led = setup_led(GPIO_NUM_2);
```

---

### `void led_out(int gpio_pin, bool on)`

Sets a GPIO LED on or off.

```c
led_out(GPIO_NUM_2, true);   // on
led_out(GPIO_NUM_2, false);  // off
```

---

### `ledc_channel_t setup_dimmable_led(int gpio_pin, ledc_channel_t channel)`

Configures a GPIO pin for PWM output using the LEDC peripheral.

| Setting | Value |
|---------|-------|
| Frequency | 5 kHz |
| Resolution | 8-bit (duty 0 – 255) |
| Speed mode | `LEDC_LOW_SPEED_MODE` |

Returns the LEDC channel handle to pass to `dimmable_led_out()`.

```c
ledc_channel_t lamp = setup_dimmable_led(GPIO_NUM_27, LEDC_CHANNEL_0);
```

---

### `void dimmable_led_out(ledc_channel_t channel, int brightness)`

Sets the PWM duty cycle. `brightness` is clamped to `0 – 255`.

- `0` = fully off
- `255` = fully on

```c
dimmable_led_out(lamp, 128); // ~50% brightness
```

---

### `void led_indicate_network_status(int gpio_pin, uint8_t status)`

Blinks or holds a GPIO LED to indicate the current network state. Blocks for one complete blink pattern before returning (~100 – 500 ms depending on state).

| `status` | Pattern | Meaning |
|----------|---------|---------|
| `0` | Short blink (50 ms on / 950 ms off) | Connecting |
| `1` | Solid on | Connected |
| `2` | Slow blink (500 ms on / 500 ms off) | Syncing NTP |
| `3` | Double burst (2 × 100 ms on) | Fetching data |
| `4` | Off | IP obtained / idle |
| `5+` | Fast blink (100 ms on / 100 ms off) | Error / failed |

```c
// In a loop — call repeatedly so the pattern keeps running
led_indicate_network_status(GPIO_NUM_2, network_status);
```

---

## Example — blink task

```c
#include "led.h"
#include "utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void blink_task(void *arg) {
    int led = setup_led(GPIO_NUM_2);
    while (1) {
        led_out(led, true);
        delay(500);
        led_out(led, false);
        delay(500);
    }
}

void app_main(void) {
    xTaskCreate(blink_task, "blink", 2048, NULL, 5, NULL);
}
```

## Example — dimmable lamp

```c
#include "led.h"

void app_main(void) {
    ledc_channel_t lamp = setup_dimmable_led(GPIO_NUM_27, LEDC_CHANNEL_0);

    // Fade up
    for (int b = 0; b <= 255; b++) {
        dimmable_led_out(lamp, b);
        delay(10);
    }
}
```

---

## Notes

- Each `setup_dimmable_led()` call consumes one LEDC channel and one LEDC timer. ESP32 has 8 low-speed channels and 4 low-speed timers.
- `led_indicate_network_status()` is designed to be called in a tight loop from a dedicated task; it contains its own `delay()` calls.
