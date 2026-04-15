#include "freertos/FreeRTOS.h"

int MAX(int a, int b) { return (a > b) ? a : b; }
int MIN(int a, int b) { return (a < b) ? a : b; }

void delay(int ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}
