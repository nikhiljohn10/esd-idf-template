#ifndef WATER_LEVEL_H
#define WATER_LEVEL_H

#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Configuration for an analog water-level sensor.
     *
     * A typical resistive analog water-level sensor (e.g. Funduino / generic
     * "water sensor module") behaves like a variable resistor whose resistance
     * decreases as more of its conductive traces are submerged. Wired to a
     * voltage divider it produces an analog voltage that rises with depth.
     */
    typedef struct
    {
        adc_channel_t channel; /*!< ADC1 channel connected to the sensor's signal pin */
        int raw_dry;           /*!< Raw 12-bit ADC reading when the sensor is fully out of water */
        int raw_full;          /*!< Raw 12-bit ADC reading when the sensor is fully submerged */
        int sample_count;      /*!< Number of samples averaged per read (>=1, e.g. 10) */
        int sample_delay_ms;   /*!< Delay between samples in ms (e.g. 10) */
    } water_level_config_t;

    /**
     * @brief A single water-level measurement.
     */
    typedef struct
    {
        int raw;     /*!< Averaged raw 12-bit ADC value (0–4095) */
        int percent; /*!< Water level as 0–100 % (clamped to range) */
    } water_level_reading_t;

    /**
     * @brief Initialise ADC1 for the configured water-level sensor channel.
     *
     * Uses 12-bit resolution with `ADC_ATTEN_DB_12` (~0–3.1 V input range),
     * matching the typical 3.3 V output of these sensor boards.
     *
     * @param[in]  config  Sensor configuration (must remain valid for the lifetime of the handle).
     * @param[out] handle  Receives the ADC oneshot unit handle.
     * @return ESP_OK on success, otherwise an esp_err_t error code.
     */
    esp_err_t water_level_init(const water_level_config_t *config,
                               adc_oneshot_unit_handle_t *handle);

    /**
     * @brief Take a single averaged reading and convert it to a 0–100 % depth.
     *
     * Averages `config->sample_count` raw samples spaced `sample_delay_ms`
     * apart, then linearly maps the result between `raw_dry` and `raw_full`.
     * The percent value is clamped to [0, 100].
     *
     * @param[in]  handle  Handle returned by `water_level_init()`.
     * @param[in]  config  Same configuration used at init.
     * @param[out] out     Receives the raw and percent values.
     * @return ESP_OK on success, otherwise an esp_err_t error code.
     */
    esp_err_t water_level_read(adc_oneshot_unit_handle_t handle,
                               const water_level_config_t *config,
                               water_level_reading_t *out);

#ifdef __cplusplus
}
#endif

#endif /* WATER_LEVEL_H */
