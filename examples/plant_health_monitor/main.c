#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "utils.h"
#include "net.h"
#include "oled.h"
#include "bme280.h"
#include "esp_log.h"

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define I2C_FREQ_HZ 100000

#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
#define TIMEZONE "IST-5:30"

static SemaphoreHandle_t bme_mutex;
oled_handle_t screen;
bme280_data_t *bme = NULL;

void screen_task(void *arg)
{
    float temp = 0.0f, rh = 0.0f;
    while (1)
    {
        if (!bme)
        {
            oled_clear(screen);
            set_text(screen, "BME280 read error", 0, 12);
            oled_show(screen);
            delay(17);
            continue;
        }

        if (xSemaphoreTake(bme_mutex, 0) != pdTRUE)
        {
            delay(17);
            continue; // mutex busy — skip frame, retry next tick
        }
        temp = bme->temperature;
        rh = bme->humidity;
        xSemaphoreGive(bme_mutex);

        char line1[18], line2[18];
        snprintf(line1, sizeof(line1), "T=%.1f%cC RH=%.0f%%", temp, UNIT_TEMP_C, rh);
        snprintf(line2, sizeof(line2), "VPD=%.2f kPa", get_vpd(temp, rh));
        oled_clear(screen);
        set_text_scroll(screen, "Plant Health Monitor", 0, 20);
        set_text(screen, line1, 0, 12);
        set_text(screen, line2, 0, 24);
        oled_show(screen);
        delay(17);
    }
}

void app_main(void)
{
    ESP_LOGI("MAIN", "Plant Health Monitor -- starting");
    delay(1000); // Allow serial monitor to attach

    /* ── OLED display (SSD1306 128×32) ────────────────────────────────── */
    screen = setup_screen(&(oled_config_t){
        .i2c_port = I2C_NUM_0,
        .sda_pin = I2C_SDA_PIN,
        .scl_pin = I2C_SCL_PIN,
    });

    /* ── Network: WiFi + NTP as background tasks ───────────────────────── */
    ESP_ERROR_CHECK(net_start(WIFI_SSID, WIFI_PASSWORD, TIMEZONE));

    /* ── BME280 sensor ────────────────────────────────────────────────── */
    bme_mutex = xSemaphoreCreateMutex();
    bme280_init(&(bme280_config_t){
                    .i2c_port = I2C_NUM_0,
                    .sda_pin = I2C_SDA_PIN,
                    .scl_pin = I2C_SCL_PIN,
                    .freq_hz = I2C_FREQ_HZ,
                },
                &bme);

    /* ── Main tasks ─────────────────────────────────────────────────────────── */
    xTaskCreate(screen_task, "screen_task", 4096, NULL, 5, NULL);

    /* ── Main loop (runs in app_main) ───────────────────────────────────────── */
    while (1)
    {
        if (bme)
        {
            xSemaphoreTake(bme_mutex, portMAX_DELAY);
            bme280_read(bme);
            xSemaphoreGive(bme_mutex);
        }
        delay(10000);
    }
}
