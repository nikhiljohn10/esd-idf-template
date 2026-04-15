#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "ssd1306.h"
#include "oled.h"
#include "adc.h"
#include "led.h"
#include "utils.h"
#include "dict.h"
#include "net.h"
#include "http.h"
#include "json.h"
#include "config.h"

portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED;
int led_brightness;
char value_buffer[32];
char ip[64] = "0.0.0.0";
uint8_t network_status = 0;

static void screen_task(void *arg)
{
    ssd1306_handle_t ssd1306_handle = setup_screen();
    while (1)
    {
        brightness_control(ssd1306_handle, led_brightness, 5);
        char line3[128] = {0};
        switch (network_status)
        {
        case 1:  snprintf(line3, sizeof(line3), "Connected!");       break;
        case 2:  snprintf(line3, sizeof(line3), "Syncing NTP...");   break;
        case 3:  snprintf(line3, sizeof(line3), "Fetching IP...");   break;
        case 4:  snprintf(line3, sizeof(line3), "%s", ip);           break;
        case 5:
            snprintf(line3, sizeof(line3), "Failed to connect");
            network_status = 0;
            break;
        default: snprintf(line3, sizeof(line3), "Connecting...");    break;
        }
        wifi_icon_state_t wifi_state =
            (network_status == 5) ? WIFI_ICON_FAILED :
            (network_status == 0) ? WIFI_ICON_CONNECTING :
            WIFI_ICON_CONNECTED;
        oled_render_home(ssd1306_handle, "LED Lamp!", wifi_state, line3, led_brightness);
        delay(25);
    }
}

static void adc_task(void *arg)
{
    int raw_light_value;
    adc_channel_t channel = ADC_CHANNEL_7;
    adc_oneshot_unit_handle_t adc_handle = setup_light_sensor(channel);
    while (1)
    {
        int avg_val = get_sample_value(adc_handle, channel, &raw_light_value); // 100ms delay
        taskENTER_CRITICAL(&my_spinlock);
        sprintf(value_buffer, "Light: %d", avg_val);
        led_brightness = MAX(0, 255 - ((avg_val * 255) / 3000)); // Scale to 0-255
        taskEXIT_CRITICAL(&my_spinlock);
    }
}

static void led_task(void *arg)
{
    ledc_channel_t lamp = setup_dimmable_led(GPIO_NUM_27, LEDC_CHANNEL_0);
    int bulb = setup_led(GPIO_NUM_2);
    while (1)
    {
        dimmable_led_out(lamp, led_brightness);
        led_indicate_network_status(bulb, network_status);
    }
}

static void http_task()
{
    // State 0 is already set (global init) — "Connecting..."
    if (setup_wifi(WIFI_SSID, WIFI_PASSWORD) != ESP_OK)
    {
        network_status = 5; // "Failed to connect" -> screen resets to 0
        vTaskDelete(NULL);
        return;
    }
    taskENTER_CRITICAL(&my_spinlock);
    network_status = 1; // "Connected!" — set immediately, before NTP blocks
    taskEXIT_CRITICAL(&my_spinlock);
    delay(500);
    taskENTER_CRITICAL(&my_spinlock);
    network_status = 2; // "Syncing NTP..." — set while waiting for NTP sync
    taskEXIT_CRITICAL(&my_spinlock);
    setup_network(); // NTP sync (may take up to 10s)

    while (1)
    {
        taskENTER_CRITICAL(&my_spinlock);
        network_status = 3; // "Fetching IP..." — set while making HTTP request for IP
        taskEXIT_CRITICAL(&my_spinlock);
        http_response_t resp;
        if (http_get("https://api.ipify.org?format=json", NULL, 0, &resp) == ESP_OK)
        {
            dict_t *json = json_parse((const char *)resp.body);
            if (json)
            {
                char new_ip[64];
                if (dict_get(json, "ip", new_ip, sizeof(new_ip)))
                {
                    taskENTER_CRITICAL(&my_spinlock);
                    strncpy(ip, new_ip, sizeof(ip) - 1);
                    ip[sizeof(ip) - 1] = '\0';
                    network_status = 4; // show IP
                    taskEXIT_CRITICAL(&my_spinlock);
                    printf("My IP: %s\n", ip);
                }
                dict_destroy(json);
            }
            free(resp.body);
        }
        else
        {
            taskENTER_CRITICAL(&my_spinlock);
            network_status = 5; // "Failed" -> screen resets to 0
            taskEXIT_CRITICAL(&my_spinlock);
        }
        delay(30000);
    }
}

void app_main(void)
{
    delay(1000); // Wait for system to stabilize

    xTaskCreate(led_task, "light", 2048, NULL, 5, NULL);
    xTaskCreate(adc_task, "sensor", 3072, NULL, 5, NULL);
    xTaskCreate(screen_task, "screen", 4096, NULL, 5, NULL);
    http_task();
}
