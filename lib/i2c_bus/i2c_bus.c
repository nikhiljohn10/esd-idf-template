/*
 * Shared I2C master bus manager for ESP-IDF 5.x.
 *
 * Maintains one i2c_master_bus_handle_t per I2C port so that multiple drivers
 * (e.g. BME280 and SSD1306 OLED) can share the same SDA/SCL pins without each
 * driver independently calling i2c_new_master_bus() and hitting ESP_ERR_INVALID_STATE.
 */

#include "i2c_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "I2C_BUS";

/* ESP32 has at most 2 I2C controllers (port 0 and port 1). */
#define I2C_BUS_MAX_PORTS 2

typedef struct
{
    i2c_master_bus_handle_t handle;
    bool initialized;
} i2c_bus_slot_t;

static i2c_bus_slot_t s_slots[I2C_BUS_MAX_PORTS];

/* Statically allocated mutex — no heap required. */
static SemaphoreHandle_t s_mutex = NULL;
static StaticSemaphore_t s_mutex_buf;

/* Returns the mutex, creating it on the first call.
 * The very first call is expected to happen from a single-threaded context
 * (e.g. app_main before any xTaskCreate) so the lazy init is safe. */
static SemaphoreHandle_t get_mutex(void)
{
    if (s_mutex == NULL)
    {
        s_mutex = xSemaphoreCreateMutexStatic(&s_mutex_buf);
    }
    return s_mutex;
}

esp_err_t i2c_bus_get_or_create(i2c_port_num_t port, int sda_pin, int scl_pin,
                                i2c_master_bus_handle_t *out_handle)
{
    if (!out_handle || port < 0 || port >= I2C_BUS_MAX_PORTS)
    {
        return ESP_ERR_INVALID_ARG;
    }

    SemaphoreHandle_t mutex = get_mutex();
    xSemaphoreTake(mutex, portMAX_DELAY);

    esp_err_t ret = ESP_OK;
    i2c_bus_slot_t *slot = &s_slots[port];

    if (slot->initialized)
    {
        ESP_LOGD(TAG, "Reusing I2C bus on port %d", port);
        *out_handle = slot->handle;
    }
    else
    {
        i2c_master_bus_config_t bus_cfg = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .i2c_port = port,
            .sda_io_num = sda_pin,
            .scl_io_num = scl_pin,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };
        ret = i2c_new_master_bus(&bus_cfg, &slot->handle);
        if (ret == ESP_OK)
        {
            slot->initialized = true;
            *out_handle = slot->handle;
            ESP_LOGI(TAG, "Created I2C bus on port %d (SDA=%d SCL=%d)", port, sda_pin, scl_pin);
        }
        else
        {
            ESP_LOGE(TAG, "i2c_new_master_bus failed on port %d: %s", port, esp_err_to_name(ret));
        }
    }

    xSemaphoreGive(mutex);
    return ret;
}
