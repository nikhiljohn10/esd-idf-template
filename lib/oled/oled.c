#include "ssd1306.h"
#include "oled.h"
#include "oled_anim.h"
#include "i2c_bus.h"
#include "font_latin_8x8.h"
#include <esp_timer.h>
#include <string.h>
#include <stdlib.h>

struct oled_context_t
{
    ssd1306_handle_t ssd1306;
};

#define TOLERANCE 10

static int last_value = INT32_MIN;
static int64_t last_checked_time_us = 0;
static uint8_t current_contrast = 0xff;
static const uint8_t OLED_ACTIVE_CONTRAST = 0xff;
static const uint8_t OLED_IDLE_CONTRAST = 0x01;
const uint8_t zero_buf[4 * 128] = {0};
uint8_t *clear_screen_buffer = (uint8_t *)zero_buf;

oled_handle_t setup_screen(const oled_config_t *config)
{
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_bus_get_or_create(config->i2c_port, config->sda_pin,
                                          config->scl_pin, &bus_handle));

    /* Probe both common SSD1306 I2C addresses */
    uint8_t addr = 0;
    if (i2c_master_probe(bus_handle, 0x3C, 50) == ESP_OK)
        addr = 0x3C;
    else if (i2c_master_probe(bus_handle, 0x3D, 50) == ESP_OK)
        addr = 0x3D;
    else
        return NULL; /* no display found */

    oled_handle_t ctx = calloc(1, sizeof(struct oled_context_t));
    if (!ctx)
        return NULL;

    ssd1306_config_t ssd1306_config = I2C_SSD1306_128x32_CONFIG_DEFAULT;
    ssd1306_config.i2c_address = addr;
    ESP_ERROR_CHECK(ssd1306_init(bus_handle, &ssd1306_config, &ctx->ssd1306));
    ssd1306_set_contrast(ctx->ssd1306, OLED_IDLE_CONTRAST);

    return ctx;
}

void oled_clear(oled_handle_t handle)
{
    ssd1306_set_pages(handle->ssd1306, clear_screen_buffer);
}

void oled_show(oled_handle_t handle)
{
    ssd1306_display_pages(handle->ssd1306);
}

void oled_pixel_test(oled_handle_t handle, uint32_t hold_ms)
{
    oled_anim_all_on(handle, hold_ms);
    oled_anim_checkerboard(handle, hold_ms);
    oled_anim_vertical_stripes(handle, hold_ms);
    oled_anim_horizontal_stripes(handle, hold_ms);
    oled_anim_concentric_rects(handle, hold_ms);

    /* Leave screen blank */
    oled_clear(handle);
    oled_show(handle);
}

void set_text(oled_handle_t handle, const char *text, int xpos, int ypos)
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
                    ssd1306_set_pixel(handle->ssd1306, xpos + i * 8 + col, ypos + bit, false);
                }
            }
        }
    }
}

esp_err_t brightness_control(oled_handle_t handle, int value, int timeout_seconds)
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
        return ssd1306_set_contrast(handle->ssd1306, current_contrast);
    }

    if (value < last_value - TOLERANCE || value > last_value + TOLERANCE)
    {
        last_value = value;
        last_checked_time_us = now_us;
        if (current_contrast != OLED_ACTIVE_CONTRAST)
        {
            current_contrast = OLED_ACTIVE_CONTRAST;
            return ssd1306_set_contrast(handle->ssd1306, current_contrast);
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
            return ssd1306_set_contrast(handle->ssd1306, current_contrast);
        }
    }

    return ESP_OK;
}

/* ── Drawing wrappers ──────────────────────────── */

esp_err_t oled_set_pixel(oled_handle_t handle, uint8_t x, uint8_t y, bool invert)
{
    return ssd1306_set_pixel(handle->ssd1306, x, y, invert);
}

esp_err_t oled_draw_line(oled_handle_t handle, uint8_t x0, uint8_t y0,
                         uint8_t x1, uint8_t y1, bool invert)
{
    esp_err_t ret = ssd1306_set_line(handle->ssd1306, x0, y0, x1, y1, invert);
    if (ret != ESP_OK)
        return ret;
    return ssd1306_display_pages(handle->ssd1306);
}

esp_err_t oled_draw_rect(oled_handle_t handle, uint8_t x, uint8_t y,
                         uint8_t w, uint8_t h, bool invert)
{
    return ssd1306_display_rectangle(handle->ssd1306, x, y, w, h, invert);
}

esp_err_t oled_fill_rect(oled_handle_t handle, uint8_t x, uint8_t y,
                         uint8_t w, uint8_t h, bool invert)
{
    return ssd1306_display_filled_rectangle(handle->ssd1306, x, y, w, h, invert);
}

esp_err_t oled_draw_circle(oled_handle_t handle, uint8_t x0, uint8_t y0,
                           uint8_t r, bool invert)
{
    return ssd1306_display_circle(handle->ssd1306, x0, y0, r, invert);
}

esp_err_t oled_fill_circle(oled_handle_t handle, uint8_t x0, uint8_t y0,
                           uint8_t r, bool invert)
{
    return ssd1306_display_filled_circle(handle->ssd1306, x0, y0, r, invert);
}

esp_err_t oled_draw_bitmap(oled_handle_t handle, uint8_t x, uint8_t y,
                           const uint8_t *bitmap, uint8_t w, uint8_t h,
                           bool invert)
{
    return ssd1306_display_bitmap(handle->ssd1306, x, y, bitmap, w, h, invert);
}

esp_err_t oled_flush(oled_handle_t handle)
{
    return ssd1306_display_pages(handle->ssd1306);
}

/**
 * Render scrolling text — non-blocking, call once per frame.
 *
 * Text starts at the left edge (x=0) and scrolls leftward at `speed`
 * pixels per second.  If the text fits entirely within the 128 px screen
 * width it is rendered statically; no scrolling occurs.
 *
 * State is tracked internally with static variables so the caller simply
 * calls this function every loop iteration alongside other set_text calls.
 */
void set_text_scroll(oled_handle_t handle, const char *text, int ypos, int speed)
{
    if (!handle || !text || speed <= 0)
        return;

    int len = strnlen(text, 64);
    int text_width = len * 8;

    /* 0 = wait at left, 1 = scrolling, 2 = wait at right */
    static int scroll_state = 0;
    static int scroll_offset = 0;
    static int64_t last_scroll_time_us = 0;
    static int64_t wait_start_us = 0;

    /* Text fits on screen — render statically, no scrolling needed */
    if (text_width <= 128)
    {
        for (int i = 0; i < len; i++)
        {
            const uint8_t *col_data = font_latin_8x8_tr[(uint8_t)text[i]];
            for (int col = 0; col < 8; col++)
                for (int bit = 0; bit < 8; bit++)
                    if (col_data[col] & (1 << bit))
                        ssd1306_set_pixel(handle->ssd1306, i * 8 + col, ypos + bit, false);
        }
        return;
    }

    int64_t now_us = esp_timer_get_time();
    int max_offset = text_width - 128;

    if (scroll_state == 0) /* waiting at left edge */
    {
        if (wait_start_us == 0)
            wait_start_us = now_us;
        if (now_us - wait_start_us >= 1000000LL)
        {
            scroll_state = 1;
            last_scroll_time_us = now_us;
        }
    }
    else if (scroll_state == 1) /* scrolling left */
    {
        int64_t elapsed_us = now_us - last_scroll_time_us;
        int pixels_to_advance = (int)(elapsed_us * speed / 1000000LL);
        if (pixels_to_advance > 0)
        {
            scroll_offset += pixels_to_advance;
            last_scroll_time_us += (int64_t)pixels_to_advance * 1000000LL / speed;
            if (scroll_offset >= max_offset)
            {
                scroll_offset = max_offset;
                scroll_state = 2;
                wait_start_us = now_us;
            }
        }
    }
    else /* waiting at right edge */
    {
        if (now_us - wait_start_us >= 1000000LL)
        {
            scroll_state = 0;
            scroll_offset = 0;
            wait_start_us = 0;
            last_scroll_time_us = 0;
        }
    }

    /* Render at current offset */
    int xpos = -scroll_offset;
    for (int i = 0; i < len; i++)
    {
        int char_x = xpos + i * 8;
        if (char_x + 8 <= 0 || char_x >= 128)
            continue;

        const uint8_t *col_data = font_latin_8x8_tr[(uint8_t)text[i]];
        for (int col = 0; col < 8; col++)
        {
            int px = char_x + col;
            if (px < 0 || px >= 128)
                continue;

            for (int bit = 0; bit < 8; bit++)
                if (col_data[col] & (1 << bit))
                    ssd1306_set_pixel(handle->ssd1306, px, ypos + bit, false);
        }
    }
}
