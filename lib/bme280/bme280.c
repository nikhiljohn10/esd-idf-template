/*
 * BME280 I2C driver for ESP-IDF 5.x
 * Supports 7SEMI ES-13528 and all BME280-compatible sensors (chip ID 0x60).
 *
 * Compensation formulae from BME280 datasheet section 4.2.3.
 */

#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bme280.h"
#include "i2c_bus.h"

static const char *TAG = "BME280";

/* ── Register map ─────────────────────────────────────────────────────────── */
#define REG_ID 0xD0
#define REG_RESET 0xE0
#define REG_CTRL_HUM 0xF2
#define REG_STATUS 0xF3
#define REG_CTRL_MEAS 0xF4
#define REG_CONFIG 0xF5
#define REG_PRESS_MSB 0xF7
#define REG_CALIB00 0x88 /* 26 bytes: 0x88-0xA1 */
#define REG_CALIB26 0xE1 /*  7 bytes: 0xE1-0xE7 */

#define BME280_CHIP_ID 0x60
#define BME280_ADDR_PRIM 0x76
#define BME280_ADDR_SEC 0x77

/* ── Driver context (one static instance) ────────────────────────────────── */
typedef struct
{
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t dev;

    /* Temperature calibration */
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;

    /* Pressure calibration */
    uint16_t dig_P1;
    int16_t dig_P2, dig_P3, dig_P4, dig_P5;
    int16_t dig_P6, dig_P7, dig_P8, dig_P9;

    /* Humidity calibration */
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;

    /* Shared intermediate for pressure/humidity compensation */
    int32_t t_fine;

    bool ready;
} bme280_ctx_t;

static bme280_ctx_t s_ctx;

/* ── Low-level I2C helpers ───────────────────────────────────────────────── */

static esp_err_t write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(s_ctx.dev, buf, 2, 1000);
}

static esp_err_t read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(s_ctx.dev, &reg, 1, data, len, 1000);
}

/* ── Calibration ─────────────────────────────────────────────────────────── */

static esp_err_t read_calibration(void)
{
    uint8_t c1[26], c2[7];
    esp_err_t err;

    err = read_regs(REG_CALIB00, c1, sizeof(c1));
    if (err != ESP_OK)
        return err;

    err = read_regs(REG_CALIB26, c2, sizeof(c2));
    if (err != ESP_OK)
        return err;

    s_ctx.dig_T1 = (uint16_t)(c1[0] | (c1[1] << 8));
    s_ctx.dig_T2 = (int16_t)(c1[2] | (c1[3] << 8));
    s_ctx.dig_T3 = (int16_t)(c1[4] | (c1[5] << 8));

    s_ctx.dig_P1 = (uint16_t)(c1[6] | (c1[7] << 8));
    s_ctx.dig_P2 = (int16_t)(c1[8] | (c1[9] << 8));
    s_ctx.dig_P3 = (int16_t)(c1[10] | (c1[11] << 8));
    s_ctx.dig_P4 = (int16_t)(c1[12] | (c1[13] << 8));
    s_ctx.dig_P5 = (int16_t)(c1[14] | (c1[15] << 8));
    s_ctx.dig_P6 = (int16_t)(c1[16] | (c1[17] << 8));
    s_ctx.dig_P7 = (int16_t)(c1[18] | (c1[19] << 8));
    s_ctx.dig_P8 = (int16_t)(c1[20] | (c1[21] << 8));
    s_ctx.dig_P9 = (int16_t)(c1[22] | (c1[23] << 8));
    /* c1[24] is reserved; c1[25] == 0xA1 == dig_H1 */
    s_ctx.dig_H1 = c1[25];

    s_ctx.dig_H2 = (int16_t)(c2[0] | (c2[1] << 8));
    s_ctx.dig_H3 = c2[2];
    s_ctx.dig_H4 = (int16_t)(((int8_t)c2[3] << 4) | (c2[4] & 0x0F));
    s_ctx.dig_H5 = (int16_t)(((int8_t)c2[5] << 4) | (c2[4] >> 4));
    s_ctx.dig_H6 = (int8_t)c2[6];

    return ESP_OK;
}

/* ── Compensation (datasheet §4.2.3) ────────────────────────────────────── */

static int32_t compensate_T(int32_t adc_T)
{
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)s_ctx.dig_T1 << 1))) *
                    ((int32_t)s_ctx.dig_T2)) >>
                   11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)s_ctx.dig_T1)) *
                      ((adc_T >> 4) - ((int32_t)s_ctx.dig_T1))) >>
                     12) *
                    ((int32_t)s_ctx.dig_T3)) >>
                   14;
    s_ctx.t_fine = var1 + var2;
    return (s_ctx.t_fine * 5 + 128) >> 8; /* 0.01 °C */
}

static uint32_t compensate_P(int32_t adc_P)
{
    int64_t var1 = ((int64_t)s_ctx.t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)s_ctx.dig_P6;
    var2 += (var1 * (int64_t)s_ctx.dig_P5) << 17;
    var2 += ((int64_t)s_ctx.dig_P4) << 35;
    var1 = ((var1 * var1 * (int64_t)s_ctx.dig_P3) >> 8) +
           ((var1 * (int64_t)s_ctx.dig_P2) << 12);
    var1 = ((((int64_t)1 << 47) + var1)) * ((int64_t)s_ctx.dig_P1) >> 33;
    if (var1 == 0)
        return 0;
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)s_ctx.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)s_ctx.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)s_ctx.dig_P7) << 4);
    return (uint32_t)p; /* Q24.8 Pa */
}

static uint32_t compensate_H(int32_t adc_H)
{
    int32_t v = s_ctx.t_fine - (int32_t)76800;
    v = (((((adc_H << 14) -
            (((int32_t)s_ctx.dig_H4) << 20) -
            (((int32_t)s_ctx.dig_H5) * v)) +
           (int32_t)16384) >>
          15) *
         (((((((v * (int32_t)s_ctx.dig_H6) >> 10) *
              (((v * (int32_t)s_ctx.dig_H3) >> 11) + (int32_t)32768)) >>
             10) +
            (int32_t)2097152) *
               (int32_t)s_ctx.dig_H2 +
           8192) >>
          14));
    v -= (((((v >> 15) * (v >> 15)) >> 7) * (int32_t)s_ctx.dig_H1) >> 4);
    if (v < 0)
        v = 0;
    if (v > 419430400)
        v = 419430400;
    return (uint32_t)(v >> 12); /* Q22.10 %RH */
}

/* ── Public API ──────────────────────────────────────────────────────────── */

esp_err_t bme280_init(const bme280_config_t *config)
{
    if (!config)
        return ESP_ERR_INVALID_ARG;

    memset(&s_ctx, 0, sizeof(s_ctx));

    /* Obtain the shared I2C master bus for this port. */
    esp_err_t err = i2c_bus_get_or_create(config->i2c_port, config->sda_pin,
                                          config->scl_pin, &s_ctx.bus);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c_bus_get_or_create failed: %s", esp_err_to_name(err));
        return err;
    }

    /* Auto-detect sensor address */
    uint8_t addrs[2] = {BME280_ADDR_PRIM, BME280_ADDR_SEC};
    err = ESP_ERR_NOT_FOUND;
    for (int i = 0; i < 2; i++)
    {
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addrs[i],
            .scl_speed_hz = config->freq_hz,
        };
        if (i2c_master_bus_add_device(s_ctx.bus, &dev_cfg, &s_ctx.dev) != ESP_OK)
            continue;

        uint8_t id = 0;
        if (read_regs(REG_ID, &id, 1) == ESP_OK && id == BME280_CHIP_ID)
        {
            ESP_LOGI(TAG, "BME280 found at 0x%02X (chip id 0x%02X)", addrs[i], id);
            err = ESP_OK;
            break;
        }
        ESP_LOGW(TAG, "No BME280 at 0x%02X (id=0x%02X)", addrs[i], id);
        i2c_master_bus_rm_device(s_ctx.dev);
        s_ctx.dev = NULL;
    }
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "BME280 not detected on I2C bus");
        return err;
    }

    /* Soft reset and wait for NVM copy */
    write_reg(REG_RESET, 0xB6);
    vTaskDelay(pdMS_TO_TICKS(10));
    uint8_t status;
    for (int i = 0; i < 50; i++)
    {
        if (read_regs(REG_STATUS, &status, 1) == ESP_OK && !(status & 0x01))
            break;
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    /* Read factory calibration data */
    err = read_calibration();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Calibration read failed: %s", esp_err_to_name(err));
        return err;
    }

    /* Configure: osrs_h=x1 (must be set before ctrl_meas), filter off */
    err = write_reg(REG_CTRL_HUM, 0x01);
    if (err != ESP_OK)
        return err;
    err = write_reg(REG_CONFIG, 0x00);
    if (err != ESP_OK)
        return err;

    s_ctx.ready = true;
    ESP_LOGI(TAG, "Initialised (SDA=%d SCL=%d %" PRIu32 "Hz)",
             config->sda_pin, config->scl_pin, config->freq_hz);
    return ESP_OK;
}

float get_vpd(float temp_c, float hum_percent)
{
    const float svp = 0.61078f * expf((17.27f * temp_c) / (temp_c + 237.3f));
    const float avp = svp * (hum_percent / 100.0f);
    return svp - avp; // in kPa
}

float get_vapor_pressure(float temp_c, float hum_percent)
{
    const float svp = 0.61078f * expf((17.27f * temp_c) / (temp_c + 237.3f));
    return svp * (hum_percent / 100.0f); // in kPa
}

float get_dew_point(float temp_c, float hum_percent)
{
    float rh = hum_percent;
    if (rh <= 0.0f)
        rh = 0.1f;
    else if (rh > 100.0f)
        rh = 100.0f;

    const float a = 17.27f;
    const float b = 237.7f;
    const float alpha = logf(rh / 100.0f) + (a * temp_c) / (b + temp_c);
    return (b * alpha) / (a - alpha);
}

float get_humidex(float temp_c, float hum_percent)
{
    const float dew_c = get_dew_point(temp_c, hum_percent);
    const float e = 6.11f * expf(5417.7530f * ((1.0f / 273.16f) -
                                               (1.0f / (dew_c + 273.16f))));
    return temp_c + 0.5555f * (e - 10.0f);
}

float get_enthalpy(float temp_c, float hum_percent, float pressure_hpa)
{
    const float pv = get_vapor_pressure(temp_c, hum_percent); // kPa
    const float p = pressure_hpa / 10.0f;                     // convert hPa to kPa
    const float w = (p > pv) ? (0.622f * pv) / (p - pv) : 0.0f;
    return 1.006f * temp_c + w * (2501.0f + 1.86f * temp_c);
}

float get_absolute_humidity(float temp_c, float hum_percent)
{
    const float svp = 6.112f * expf((17.67f * temp_c) / (temp_c + 243.5f));
    const float avp = svp * (hum_percent / 100.0f); // in hPa
    return 216.7f * avp / (temp_c + 273.15f);       // g/m^3
}

float get_heat_index(float temp_c, float hum_percent)
{
    float rh = hum_percent;
    if (rh < 0.0f)
        rh = 0.0f;
    else if (rh > 100.0f)
        rh = 100.0f;

    float t_f = temp_c * 9.0f / 5.0f + 32.0f;
    if (t_f < 80.0f)
        return temp_c;

    float hi_f = -42.379f +
                 2.04901523f * t_f +
                 10.14333127f * rh +
                 -0.22475541f * t_f * rh +
                 -6.83783e-3f * t_f * t_f +
                 -5.481717e-2f * rh * rh +
                 1.22874e-3f * t_f * t_f * rh +
                 8.5282e-4f * t_f * rh * rh +
                 -1.99e-6f * t_f * t_f * rh * rh;

    if (rh < 13.0f && t_f >= 80.0f && t_f <= 112.0f)
    {
        hi_f -= ((13.0f - rh) * 0.25f) *
                sqrtf((17.0f - fabsf(t_f - 95.0f)) * 0.05882f);
    }
    else if (rh > 85.0f && t_f >= 80.0f && t_f <= 87.0f)
    {
        hi_f += ((rh - 85.0f) * 0.1f) * ((87.0f - t_f) * 0.2f);
    }

    return (hi_f - 32.0f) * 5.0f / 9.0f;
}

esp_err_t bme280_read(bme280_data_t *data)
{
    if (!s_ctx.ready || !data)
        return ESP_ERR_INVALID_STATE;

    /* Trigger forced-mode measurement: osrs_t=x1, osrs_p=x1, mode=01 */
    esp_err_t err = write_reg(REG_CTRL_MEAS, (0x01 << 5) | (0x01 << 2) | 0x01);
    if (err != ESP_OK)
        return err;

    /* Poll until measuring bit clears (max ~10 ms for x1/x1/x1) */
    uint8_t status;
    for (int i = 0; i < 50; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(5));
        if (read_regs(REG_STATUS, &status, 1) == ESP_OK && !(status & 0x08))
            break;
    }

    /* Burst-read 8 bytes: press[3] temp[3] hum[2] */
    uint8_t d[8];
    err = read_regs(REG_PRESS_MSB, d, sizeof(d));
    if (err != ESP_OK)
        return err;

    int32_t adc_P = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
    int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
    int32_t adc_H = ((int32_t)d[6] << 8) | (int32_t)d[7];

    /* Temperature must be compensated first — it populates t_fine */
    int32_t T = compensate_T(adc_T);
    uint32_t P = compensate_P(adc_P);
    uint32_t H = compensate_H(adc_H);

    float press_hpa = P / 25600.0f; // Pressure is in Q24.8 Pa units, convert to hPa
    data->temperature = T / 100.0f;
    data->pressure = press_hpa;
    data->pressure_bar = press_hpa / 1000.0f; // Convert hPa to bar
    data->humidity = H / 1024.0f;
    return ESP_OK;
}
