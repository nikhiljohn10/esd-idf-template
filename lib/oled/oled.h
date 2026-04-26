#ifndef OLED_H
#define OLED_H

#include "driver/i2c_master.h"

extern const uint8_t zero_buf[4 * 128];
extern uint8_t *clear_screen_buffer;

/* Forward-declare the opaque context so oled_anim.h can use oled_handle_t. */
struct oled_context_t;
typedef struct oled_context_t *oled_handle_t;

/**
 * @brief I2C pin and port configuration for the OLED display.
 */
typedef struct
{
    i2c_port_num_t i2c_port; /*!< I2C peripheral number (e.g. I2C_NUM_0) */
    int sda_pin;             /*!< GPIO number for SDA */
    int scl_pin;             /*!< GPIO number for SCL */
} oled_config_t;

/* Constants shared across modules and application code */
#include "oled_const.h"

/* Icon and animation sub-modules — re-exported so callers only need oled.h */
#include "oled_icon.h"
#include "oled_anim.h"

/**
 * @brief Fill the display with a sequence of test patterns to verify every pixel.
 *
 * Patterns shown (each held for hold_ms milliseconds):
 *   1. All pixels ON  — confirms no dead columns/rows
 *   2. Checkerboard   — reveals stuck-on/off neighbours
 *   3. Vertical stripes (2 px wide)
 *   4. Horizontal stripes (2 px tall)
 *   5. Border rectangle — checks corners and edges
 *
 * The display is cleared before returning.
 */
void oled_pixel_test(oled_handle_t handle, uint32_t hold_ms);

/** Clear the display's pixel buffer (does not push to hardware). */
void oled_clear(oled_handle_t handle);
/** Push the current pixel buffer to the display hardware. */
void oled_show(oled_handle_t handle);
void set_text(oled_handle_t handle, const char *text, int xpos, int ypos);

/**
 * Scroll text right-to-left across one row. One call = one full pass.
 * speed: pixels per second (e.g. 30 for a smooth ticker).
 * Wrap in a loop for continuous scrolling.
 */
void set_text_scroll(oled_handle_t handle, const char *text, int ypos, int speed);

esp_err_t brightness_control(oled_handle_t handle, int value, int timeout_seconds);
oled_handle_t setup_screen(const oled_config_t *config);

#endif // OLED_H
