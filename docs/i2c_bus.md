# i2c_bus

Shared I2C master bus manager for ESP-IDF 5.x.

**Header:** `lib/i2c_bus/i2c_bus.h`

---

## Overview

ESP-IDF's `i2c_master` API requires `i2c_new_master_bus()` to be called once per I2C port. When multiple libraries (e.g. `bme280` and `oled`) each try to create a bus on the same port and pins, the second call returns `ESP_ERR_INVALID_STATE` and the second driver fails to initialise.

`i2c_bus` solves this by acting as a singleton registry: it creates the bus on the first request for a port and returns the cached handle on every subsequent request. All callers sharing a port transparently receive the same `i2c_master_bus_handle_t` without any additional wiring.

---

## API

### `esp_err_t i2c_bus_get_or_create(port, sda_pin, scl_pin, &out_handle)`

```c
esp_err_t i2c_bus_get_or_create(i2c_port_num_t port,
                                 int sda_pin,
                                 int scl_pin,
                                 i2c_master_bus_handle_t *out_handle);
```

| Parameter    | Description                                             |
| ------------ | ------------------------------------------------------- |
| `port`       | I2C peripheral number (`I2C_NUM_0` or `I2C_NUM_1`)      |
| `sda_pin`    | GPIO for SDA — used only on the **first** call per port |
| `scl_pin`    | GPIO for SCL — used only on the **first** call per port |
| `out_handle` | Receives the shared `i2c_master_bus_handle_t`           |

Returns `ESP_OK` on success. All callers sharing a port must use the same physical pins; the pin arguments are silently ignored after the bus is created.

The function is task-safe (protected by an internal mutex).

---

## Usage

Libraries do not need to call `i2c_bus_get_or_create()` directly — `bme280` and `oled` call it internally using the pins from their own config structs.

If you are writing a new I2C driver, replace your `i2c_new_master_bus()` call with:

```c
#include "i2c_bus.h"

i2c_master_bus_handle_t bus;
ESP_ERROR_CHECK(i2c_bus_get_or_create(I2C_NUM_0, sda_pin, scl_pin, &bus));

// Then add your device to the shared bus as usual:
i2c_device_config_t dev_cfg = { .device_address = 0x48, .scl_speed_hz = 400000 };
i2c_master_dev_handle_t dev;
ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &dev));
```

---

## Sharing a bus between BME280 and OLED

Both the `bme280` and `oled` libraries now call `i2c_bus_get_or_create()` internally. Declare the same port and pins in both configs and the bus will be created once:

```c
#include "bme280.h"
#include "oled.h"

#define I2C_PORT  I2C_NUM_0
#define I2C_SDA   21
#define I2C_SCL   22

// BME280 on I2C_NUM_0
bme280_config_t bme_cfg = {
    .i2c_port = I2C_PORT,
    .sda_pin  = I2C_SDA,
    .scl_pin  = I2C_SCL,
    .freq_hz  = 100000,
};
bme280_init(&bme_cfg);

// OLED on the same bus — i2c_bus returns the cached handle
ssd1306_handle_t oled = setup_screen(&(oled_config_t){
    .i2c_port = I2C_PORT,
    .sda_pin  = I2C_SDA,
    .scl_pin  = I2C_SCL,
});
```

> The order of initialisation does not matter; whichever library runs first creates the bus and the other reuses it.
