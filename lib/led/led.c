
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/ledc.h"

int setup_led(int gpio_pin)
{
    gpio_reset_pin(gpio_pin);
    gpio_set_direction(gpio_pin, GPIO_MODE_OUTPUT);
    return gpio_pin;
}

void led_out(int gpio_pin, bool on)
{
    gpio_set_level(gpio_pin, (uint32_t)on);
}

void dimmable_led_out(ledc_channel_t channel, int brightness)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, brightness);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

void led_indicate_network_status(int gpio_pin, uint8_t status)
{
    if (status == 0) // Connecting: short blink
    {
        led_out(gpio_pin, true);
        vTaskDelay(pdMS_TO_TICKS(50));
        led_out(gpio_pin, false);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    else if (status == 1) // Connected: solid on
    {
        led_out(gpio_pin, true);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    else if (status == 2) // Syncing NTP: slow blink
    {
        led_out(gpio_pin, true);
        vTaskDelay(pdMS_TO_TICKS(500));
        led_out(gpio_pin, false);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    else if (status == 3) // Fetching IP: double-blink burst
    {
        led_out(gpio_pin, true);
        vTaskDelay(pdMS_TO_TICKS(50));
        led_out(gpio_pin, false);
        vTaskDelay(pdMS_TO_TICKS(50));
        led_out(gpio_pin, true);
        vTaskDelay(pdMS_TO_TICKS(50));
        led_out(gpio_pin, false);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    else // status 4+: off
    {
        led_out(gpio_pin, false);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

ledc_channel_t setup_dimmable_led(int gpio_pin, ledc_channel_t channel)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = channel,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = gpio_pin,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ledc_channel);

    return channel;
}
