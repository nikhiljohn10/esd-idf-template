#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "ssd1306.h"
#include "oled.h"
#include "water_level.h"
#include "led.h"
#include "utils.h"

/*
 * Water Tank Level Monitor
 * ------------------------
 * Reads an analog water-level sensor on GPIO34 (ADC1_CH6), shows the depth as
 * a percentage on the OLED, and lights the on-board LED when the level falls
 * below a low-water threshold.
 *
 * Calibration values below are placeholders — record the raw ADC values your
 * sensor produces when fully dry and fully submerged, then update raw_dry /
 * raw_full accordingly (see docs/water_level.md).
 *
 * Wiring:
 *   Sensor  ->  ESP32
 *   VCC     ->  3V3
 *   GND     ->  GND
 *   Signal  ->  GPIO 34 (ADC1_CH6)
 */

#define LOW_WATER_PERCENT 20

static water_level_reading_t g_reading;
static bool g_have_reading = false;

static void sensor_task(void *arg)
{
    water_level_config_t cfg = {
        .channel = ADC_CHANNEL_6, /* GPIO34 */
        .raw_dry = 100,
        .raw_full = 2800,
        .sample_count = 10,
        .sample_delay_ms = 10,
    };

    adc_oneshot_unit_handle_t adc;
    if (water_level_init(&cfg, &adc) != ESP_OK)
    {
        printf("water_level init failed\n");
        vTaskDelete(NULL);
        return;
    }

    while (1)
    {
        water_level_reading_t r;
        if (water_level_read(adc, &cfg, &r) == ESP_OK)
        {
            g_reading = r;
            g_have_reading = true;
            printf("raw=%d  level=%d%%\n", r.raw, r.percent);
        }
        delay(500);
    }
}

static void alarm_task(void *arg)
{
    int led = setup_led(GPIO_NUM_2);
    while (1)
    {
        bool low = g_have_reading && g_reading.percent < LOW_WATER_PERCENT;
        led_out(led, low);
        delay(low ? 250 : 1000);
        led_out(led, false);
        delay(250);
    }
}

static void screen_task(void *arg)
{
    oled_handle_t oled = setup_screen(&(oled_config_t){
        .i2c_port = I2C_NUM_0,
        .sda_pin = 21,
        .scl_pin = 22,
    });
    while (1)
    {
        char line[32];
        if (g_have_reading)
            snprintf(line, sizeof(line), "Level: %d%%", g_reading.percent);
        else
            snprintf(line, sizeof(line), "Reading...");

        oled_render_home(oled, "Tank", WIFI_ICON_CONNECTED, line);
        delay(250);
    }
}

void app_main(void)
{
    delay(1000);
    xTaskCreate(sensor_task, "water", 3072, NULL, 5, NULL);
    xTaskCreate(alarm_task, "alarm", 2048, NULL, 5, NULL);
    xTaskCreate(screen_task, "screen", 4096, NULL, 5, NULL);
}
