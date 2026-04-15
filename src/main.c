#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "utils.h"
#include "led.h"

// Uncomment the libraries your project needs:
// #include "adc.h"
// #include "net.h"
// #include "http.h"
// #include "json.h"
// #include "dict.h"
// #include "oled.h"
// #include "config.h"   // WiFi credentials -- copy config.h.example first

#define STATUS_LED_PIN GPIO_NUM_2   // Built-in LED on most ESP32 dev boards

static void blink_task(void *arg)
{
    (void)arg;
    int led = setup_led(STATUS_LED_PIN);
    while (1)
    {
        led_out(led, true);
        delay(500);
        led_out(led, false);
        delay(500);
    }
}

void app_main(void)
{
    printf("ESP-IDF Template -- starting\n");
    delay(1000); // Allow serial monitor to attach

    // Add your FreeRTOS tasks here:
    xTaskCreate(blink_task, "blink", 2048, NULL, 5, NULL);
}
