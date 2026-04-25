/*
 * Analog water-level sensor driver.
 *
 * Reads a resistive water-level module on an ADC1 channel, averages multiple
 * samples and maps the result to a calibrated 0–100 % depth.
 */

#include "water_level.h"
#include "esp_log.h"
#include "utils.h"

static const char *TAG = "water_level";

esp_err_t water_level_init(const water_level_config_t *config,
                           adc_oneshot_unit_handle_t *handle)
{
    if (!config || !handle)
        return ESP_ERR_INVALID_ARG;
    if (config->sample_count < 1)
        return ESP_ERR_INVALID_ARG;

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(err));
        return err;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, /* 12-bit on ESP32 */
        .atten = ADC_ATTEN_DB_12,         /* ~0–3.1 V */
    };
    err = adc_oneshot_config_channel(*handle, config->channel, &chan_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "adc_oneshot_config_channel failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Initialised channel=%d dry=%d full=%d",
             (int)config->channel, config->raw_dry, config->raw_full);
    return ESP_OK;
}

esp_err_t water_level_read(adc_oneshot_unit_handle_t handle,
                           const water_level_config_t *config,
                           water_level_reading_t *out)
{
    if (!handle || !config || !out)
        return ESP_ERR_INVALID_ARG;

    int sum = 0;
    int value = 0;
    for (int i = 0; i < config->sample_count; i++)
    {
        esp_err_t err = adc_oneshot_read(handle, config->channel, &value);
        if (err != ESP_OK)
            return err;
        sum += value;
        if (config->sample_delay_ms > 0)
            delay(config->sample_delay_ms);
    }

    int avg = sum / config->sample_count;
    out->raw = avg;

    int span = config->raw_full - config->raw_dry;
    int percent;
    if (span == 0)
    {
        percent = 0;
    }
    else
    {
        percent = ((avg - config->raw_dry) * 100) / span;
        percent = MAX(0, MIN(100, percent));
    }
    out->percent = percent;
    return ESP_OK;
}
