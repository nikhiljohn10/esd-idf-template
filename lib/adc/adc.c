#include "freertos/FreeRTOS.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "utils.h"

#define SAMPLE_COUNT 10

adc_oneshot_unit_handle_t setup_light_sensor(adc_channel_t channel)
{
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, // 12-bit (0-4095)
        .atten = ADC_ATTEN_DB_12,         // Measure up to ~3.1V
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, channel, &config));

    return adc_handle;
}

int get_sample_value(adc_oneshot_unit_handle_t adc_handle, adc_channel_t channel, int *out_raw)
{
    int value;
    int sum = 0;

    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, channel, &value));
        *out_raw = value;
        sum += value;
        delay(10);
    }
    return (sum / SAMPLE_COUNT);
}
