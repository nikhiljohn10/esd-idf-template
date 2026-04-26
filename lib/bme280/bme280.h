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
    esp_err_t bme280_init(const bme280_config_t *config, bme280_data_t **data);

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

    /**
     * @brief Calculate vapor pressure deficit from temperature and humidity.
     *
     * Uses the Tetens formula to compute saturation vapor pressure from
     * temperature, then subtracts the actual vapor pressure based on relative
     * humidity.
     *
     * @param temp_c      Temperature in degrees Celsius.
     * @param hum_percent Relative humidity in percent (0–100).
     * @return Vapor pressure deficit in kilopascals (kPa).
     */
    float get_vpd(float temp_c, float hum_percent);

    /**
     * @brief Calculate actual vapor pressure from temperature and humidity.
     *
     * @param temp_c      Temperature in degrees Celsius.
     * @param hum_percent Relative humidity in percent (0–100).
     * @return Actual vapor pressure in kilopascals (kPa).
     */
    float get_vapor_pressure(float temp_c, float hum_percent);

    /**
     * @brief Calculate dew point from temperature and humidity.
     *
     * Uses the Magnus-Tetens approximation.
     *
     * @param temp_c      Temperature in degrees Celsius.
     * @param hum_percent Relative humidity in percent (0–100).
     * @return Dew point in degrees Celsius.
     */
    float get_dew_point(float temp_c, float hum_percent);

    /**
     * @brief Calculate humidex from temperature and humidity.
     *
     * Uses the Canadian humidex formula and returns the apparent temperature
     * in degrees Celsius.
     *
     * @param temp_c      Temperature in degrees Celsius.
     * @param hum_percent Relative humidity in percent (0–100).
     * @return Humidex in degrees Celsius.
     */
    float get_humidex(float temp_c, float hum_percent);

    /**
     * @brief Calculate moist-air enthalpy from temperature, humidity and pressure.
     *
     * Uses the humidity ratio derived from vapor pressure and ambient pressure.
     *
     * @param temp_c       Temperature in degrees Celsius.
     * @param hum_percent  Relative humidity in percent (0–100).
     * @param pressure_hpa Absolute pressure in hPa.
     * @return Specific enthalpy in kJ/kg of dry air.
     */
    float get_enthalpy(float temp_c, float hum_percent, float pressure_hpa);

    /**
     * @brief Calculate absolute humidity from temperature and humidity.
     *
     * Returns the mass of water vapor per cubic meter of air.
     *
     * @param temp_c      Temperature in degrees Celsius.
     * @param hum_percent Relative humidity in percent (0–100).
     * @return Absolute humidity in grams per cubic meter (g/m^3).
     */
    float get_absolute_humidity(float temp_c, float hum_percent);

    /**
     * @brief Calculate heat index from temperature and humidity.
     *
     * Uses the NOAA heat index formula with Celsius input and returns the
     * apparent temperature in degrees Celsius.
     *
     * If the temperature is below 80 °F (26.7 °C), this function returns the
     * actual air temperature because heat-index corrections are not typically
     * applied in cooler conditions.
     *
     * @param temp_c      Temperature in degrees Celsius.
     * @param hum_percent Relative humidity in percent (0–100).
     * @return Heat index in degrees Celsius.
     */
    float get_heat_index(float temp_c, float hum_percent);

#ifdef __cplusplus
}
#endif
