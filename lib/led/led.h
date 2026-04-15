#ifndef LED_H
#define LED_H

#include "driver/ledc.h"

int setup_led(int gpio_pin);
void led_out(int gpio_pin, bool on);
ledc_channel_t setup_dimmable_led(int gpio_pin, ledc_channel_t channel);
void dimmable_led_out(ledc_channel_t channel, int brightness);
void led_indicate_network_status(int gpio_pin, uint8_t status);

#endif // LED_H
