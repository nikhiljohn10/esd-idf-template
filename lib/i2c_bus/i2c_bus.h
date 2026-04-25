#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Get or create a shared I2C master bus for the given port.
     *
     * On the first call for a given port the bus is created with the supplied
     * SDA/SCL pins.  Subsequent calls for the same port return the existing
     * handle; the pin arguments are ignored so all callers that share a port
     * MUST use the same physical pins.
     *
     * This function is safe to call from multiple tasks (protected internally
     * by a mutex).
     *
     * @param[in]  port       I2C peripheral number (I2C_NUM_0 or I2C_NUM_1).
     * @param[in]  sda_pin    GPIO number for SDA (used only on first call per port).
     * @param[in]  scl_pin    GPIO number for SCL (used only on first call per port).
     * @param[out] out_handle Pointer that receives the shared bus handle.
     * @return ESP_OK on success, otherwise an esp_err_t error code.
     */
    esp_err_t i2c_bus_get_or_create(i2c_port_num_t port, int sda_pin, int scl_pin,
                                    i2c_master_bus_handle_t *out_handle);

#ifdef __cplusplus
}
#endif
