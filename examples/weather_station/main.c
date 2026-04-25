#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ssd1306.h"
#include "oled.h"
#include "bme280.h"
#include "utils.h"

/*
 * BME280 Weather Station
 * ----------------------
 * Reads temperature, pressure and humidity from a BME280 sensor every 2 s
 * and renders the values on a 128x32 SSD1306 OLED.
 *
 * Wiring (defaults — change BME280_SDA/SCL below for your board):
 *   BME280   ->  ESP32
 *   VCC      ->  3V3
 *   GND      ->  GND
 *   SDA      ->  GPIO 21
 *   SCL      ->  GPIO 22
 *
 * The OLED uses its own I2C bus (managed by the `oled` library), so the BME280
 * is placed on a separate I2C port to keep both drivers self-contained.
 */

#define BME280_I2C_PORT I2C_NUM_0
#define BME280_SDA GPIO_NUM_21
#define BME280_SCL GPIO_NUM_22

static bme280_data_t g_sample;
static bool g_have_sample = false;

static void sensor_task(void *arg)
{
    bme280_config_t cfg = {
        .i2c_port = BME280_I2C_PORT,
        .sda_pin = BME280_SDA,
        .scl_pin = BME280_SCL,
        .freq_hz = 100000,
    };
    if (bme280_init(&cfg) != ESP_OK)
    {
        printf("BME280 init failed\n");
        vTaskDelete(NULL);
        return;
    }

    while (1)
    {
        bme280_data_t s;
        if (bme280_read(&s) == ESP_OK)
        {
            g_sample = s;
            g_have_sample = true;
            printf("T=%.2f C  P=%.2f hPa  RH=%.1f %%\n",
                   s.temperature, s.pressure, s.humidity);
        }
        delay(2000);
    }
}

static void screen_task(void *arg)
{
    ssd1306_handle_t oled = setup_screen();
    while (1)
    {
        char line[32];
        if (g_have_sample)
        {
            snprintf(line, sizeof(line), "%.1fC %.0fhPa %.0f%%",
                     g_sample.temperature,
                     g_sample.pressure,
                     g_sample.humidity);
        }
        else
        {
            snprintf(line, sizeof(line), "Reading...");
        }
        oled_render_home(oled, "Weather", WIFI_ICON_CONNECTED, line, 0);
        delay(500);
    }
}

void app_main(void)
{
    delay(1000);
    xTaskCreate(sensor_task, "bme280", 3072, NULL, 5, NULL);
    xTaskCreate(screen_task, "screen", 4096, NULL, 5, NULL);
}
