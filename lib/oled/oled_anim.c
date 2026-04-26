/**
 * @file oled_anim.c
 * @brief Multi-frame animation patterns for the SSD1306 OLED display.
 *
 * Each function plays one looping or growing pattern using the public
 * oled drawing API.  They are called by oled_pixel_test() and can also
 * be used directly from application code.
 */

#include "oled_anim.h"
#include "oled.h"
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ── Animation patterns ─────────────────────────────────────────────────── */

void oled_anim_all_on(oled_handle_t handle, uint32_t hold_ms)
{
    oled_fill_rect(handle, 0, 0, 128, 32, false);
    vTaskDelay(pdMS_TO_TICKS(hold_ms));
}

void oled_anim_checkerboard(oled_handle_t handle, uint32_t hold_ms)
{
    for (int size = 1; size <= 16; size++)
    {
        oled_clear(handle);
        for (int x = 0; x < 128; x++)
        {
            for (int y = 0; y < 32; y++)
            {
                if ((((abs(x - 63) + size / 2) / size) + ((abs(y - 15) + size / 2) / size)) & 1)
                    oled_set_pixel(handle, x, y, false);
            }
        }
        oled_flush(handle);
        vTaskDelay(pdMS_TO_TICKS(hold_ms));
    }
}

void oled_anim_vertical_stripes(oled_handle_t handle, uint32_t hold_ms)
{
    for (int size = 1; size <= 16; size++)
    {
        oled_clear(handle);
        for (int x = 0; x < 128; x++)
        {
            if (((abs(x - 63) + size / 2) / size) & 1)
            {
                for (int y = 0; y < 32; y++)
                    oled_set_pixel(handle, x, y, false);
            }
        }
        oled_flush(handle);
        vTaskDelay(pdMS_TO_TICKS(hold_ms));
    }
}

void oled_anim_horizontal_stripes(oled_handle_t handle, uint32_t hold_ms)
{
    for (int size = 1; size <= 16; size++)
    {
        oled_clear(handle);
        for (int y = 0; y < 32; y++)
        {
            if (((abs(y - 15) + size / 2) / size) & 1)
            {
                for (int x = 0; x < 128; x++)
                    oled_set_pixel(handle, x, y, false);
            }
        }
        oled_flush(handle);
        vTaskDelay(pdMS_TO_TICKS(hold_ms));
    }
}

void oled_anim_concentric_rects(oled_handle_t handle, uint32_t hold_ms)
{
    for (int gap = 1;; gap++)
    {
        int step = gap + 1; /* pixels between rectangle edges: 1 drawn + gap empty */
        oled_clear(handle);
        for (int i = 0;; i++)
        {
            int x0 = i * step, x1 = 127 - i * step;
            int y0 = i * step, y1 = 31 - i * step;
            if (x0 >= x1 || y0 >= y1)
                break;
            for (int x = x0; x <= x1; x++)
            {
                oled_set_pixel(handle, x, y0, false);
                oled_set_pixel(handle, x, y1, false);
            }
            for (int y = y0 + 1; y < y1; y++)
            {
                oled_set_pixel(handle, x0, y, false);
                oled_set_pixel(handle, x1, y, false);
            }
        }
        oled_flush(handle);
        vTaskDelay(pdMS_TO_TICKS(hold_ms));
        if (step >= 16) /* gap large enough that no second rectangle fits */
            break;
    }
}
