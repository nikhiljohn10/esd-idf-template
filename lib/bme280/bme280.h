#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief BME280 I2C bus and pin configuration.
     */
    typedef struct
    {
        i2c_port_num_t i2c_port; /*!< I2C peripheral number (e.g. I2C_NUM_0) */
        int sda_pin;             /*!< GPIO number for SDA */
        int scl_pin;             /*!< GPIO number for SCL */
        uint32_t freq_hz;        /*!< I2C clock frequency in Hz (max 400000) */
    } bme280_config_t;

    /**
     * @brief Compensated sensor readings.
     */
    typedef struct
    {
        float temperature;  /*!< Temperature in degrees Celsius */
        float pressure;     /*!< Barometric pressure in hPa */
        float pressure_bar; /*!< Barometric pressure in bar */
        float humidity;     /*!< Relative humidity in %RH */
    } bme280_data_t;

    /**
     * @brief Initialise the BME280 sensor.
     *
     * Creates the I2C master bus, auto-detects the sensor at address 0x76 or
     * 0x77, performs a soft-reset, reads all factory calibration data and
     * configures the device for forced-mode operation with x1 oversampling on
     * all channels and the IIR filter disabled (weather-monitoring profile,
     * datasheet §3.5.1).
     *
     * @param[in] config  Pointer to pin/bus configuration.
     * @return ESP_OK on success, otherwise an esp_err_t error code.
     */
    esp_err_t bme280_init(const bme280_config_t *config);

    /**
     * @brief Trigger a single forced-mode measurement and return the results.
     *
     * Writes ctrl_meas to start a measurement, polls the status register until
     * the sensor returns to sleep mode, then burst-reads and compensates all
     * three channels.
     *
     * @param[out] data  Pointer to a bme280_data_t struct to be filled.
     * @return ESP_OK on success, otherwise an esp_err_t error code.
     */
    esp_err_t bme280_read(bme280_data_t *data);

#ifdef __cplusplus
}
#endif
