#include "ssd1306.h"
#include "oled.h"
#include "i2c_bus.h"
#include "font_latin_8x8.h"
#include <esp_timer.h>
#include <string.h>

#define TOLERANCE 10

static int last_value = INT32_MIN;
static int64_t last_checked_time_us = 0;
static uint8_t current_contrast = 0xff;
static const uint8_t OLED_ACTIVE_CONTRAST = 0xff;
static const uint8_t OLED_IDLE_CONTRAST = 0x01;
const uint8_t zero_buf[4 * 128] = {0};
uint8_t *clear_screen_buffer = (uint8_t *)zero_buf;

ssd1306_handle_t setup_screen(const oled_config_t *config)
{
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_bus_get_or_create(config->i2c_port, config->sda_pin,
                                          config->scl_pin, &bus_handle));

    ssd1306_handle_t handle;
    ssd1306_config_t ssd1306_config = I2C_SSD1306_128x32_CONFIG_DEFAULT;
    ESP_ERROR_CHECK(ssd1306_init(bus_handle, &ssd1306_config, &handle));
    ssd1306_set_contrast(handle, 0xff);

    return handle;
}

void set_latin_text(ssd1306_handle_t handle, const char *text, int xpos, int ypos)
{
    for (int i = 0; i < strnlen(text, 18); i++)
    {
        const uint8_t *col_data = font_latin_8x8_tr[(uint8_t)text[i]];
        for (int col = 0; col < 8; col++)
        {
            for (int bit = 0; bit < 8; bit++)
            {
                if (col_data[col] & (1 << bit))
                {
                    ssd1306_set_pixel(handle, xpos + i * 8 + col, ypos + bit, false);
                }
            }
        }
    }
}

void draw_bitmap_8x8(ssd1306_handle_t handle, const uint8_t *bitmap, int xpos, int ypos)
{
    for (int col = 0; col < 8; col++)
    {
        for (int bit = 0; bit < 8; bit++)
        {
            if (bitmap[col] & (1 << bit))
            {
                ssd1306_set_pixel(handle, xpos + col, ypos + bit, false);
            }
        }
    }
}

// Column-major 8x8 WiFi animation frames: dot → inner arc → middle arc → outer arc.
static const uint8_t s_wifi_anim_frames[4][8] = {
    {0x00, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00}, // dot only
    {0x00, 0x00, 0x00, 0x50, 0x50, 0x00, 0x00, 0x00}, // + inner arc
    {0x00, 0x08, 0x04, 0x54, 0x54, 0x04, 0x08, 0x00}, // + middle arc
    {0x02, 0x09, 0x05, 0x55, 0x55, 0x05, 0x09, 0x02}, // + outer arc (full)
};
static const uint8_t s_wifi_connected[8] = {0x02, 0x09, 0x05, 0x55, 0x55, 0x05, 0x09, 0x02};
static const uint8_t s_wifi_failed[8] = {0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81};

void draw_wifi_icon(ssd1306_handle_t handle, wifi_icon_state_t state, int xpos, int ypos)
{
    static int s_tick = 0;
    static int s_frame = 0;
    const uint8_t *bmp;
    if (state == WIFI_ICON_CONNECTING)
    {
        if (++s_tick >= 6)
        {
            s_tick = 0;
            s_frame = (s_frame + 1) % 4;
        }
        bmp = s_wifi_anim_frames[s_frame];
    }
    else if (state == WIFI_ICON_FAILED)
    {
        s_tick = 0;
        s_frame = 0;
        bmp = s_wifi_failed;
    }
    else
    {
        s_tick = 0;
        s_frame = 0;
        bmp = s_wifi_connected;
    }
    draw_bitmap_8x8(handle, bmp, xpos, ypos);
}

void oled_render_home(ssd1306_handle_t handle, const char *title,
                      wifi_icon_state_t wifi_state, const char *status_text,
                      int led_brightness)
{
    int bar_length = (255 - led_brightness) * 128 / 255;
    ssd1306_set_pages(handle, clear_screen_buffer);
    set_latin_text(handle, title, 0, 0);
    draw_wifi_icon(handle, wifi_state, 120, 0);
    set_latin_text(handle, status_text, 0, 18);
    ssd1306_display_filled_rectangle(handle, 0, 10, bar_length, 4, false);
    ssd1306_display_pages(handle);
}

// Control the screen contrast so the OLED is bright while values change,
// and dims after timeout_seconds of no changes.
esp_err_t brightness_control(ssd1306_handle_t handle, int value, int timeout_seconds)
{
    if (!handle)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int64_t now_us = esp_timer_get_time();

    if (last_checked_time_us == 0)
    {
        last_checked_time_us = now_us;
        last_value = value;
        current_contrast = OLED_ACTIVE_CONTRAST;
        return ssd1306_set_contrast(handle, current_contrast);
    }

    if (value < last_value - TOLERANCE || value > last_value + TOLERANCE)
    {
        last_value = value;
        last_checked_time_us = now_us;
        if (current_contrast != OLED_ACTIVE_CONTRAST)
        {
            current_contrast = OLED_ACTIVE_CONTRAST;
            return ssd1306_set_contrast(handle, current_contrast);
        }
        return ESP_OK;
    }

    int64_t elapsed_seconds = (now_us - last_checked_time_us) / 1000000;
    if (elapsed_seconds >= timeout_seconds)
    {
        last_checked_time_us = now_us;
        if (current_contrast != OLED_IDLE_CONTRAST)
        {
            current_contrast = OLED_IDLE_CONTRAST;
            return ssd1306_set_contrast(handle, current_contrast);
        }
    }

    return ESP_OK;
}
