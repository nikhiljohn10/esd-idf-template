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
 * Wiring (defaults — change the defines below for your board):
 *   BME280 / OLED  ->  ESP32
 *   VCC            ->  3V3
 *   GND            ->  GND
 *   SDA            ->  GPIO 21
 *   SCL            ->  GPIO 22
 *
 * Both devices share the same I2C bus (I2C_NUM_0). The i2c_bus library
 * ensures the bus is created only once regardless of initialisation order.
 */

#define I2C_PORT I2C_NUM_0
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22

static bme280_data_t g_sample;
static bool g_have_sample = false;

static void sensor_task(void *arg)
{
    bme280_config_t cfg = {
        .i2c_port = I2C_PORT,
        .sda_pin = I2C_SDA,
        .scl_pin = I2C_SCL,
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
    ssd1306_handle_t oled = setup_screen(&(oled_config_t){
        .i2c_port = I2C_PORT,
        .sda_pin = I2C_SDA,
        .scl_pin = I2C_SCL,
    });
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
