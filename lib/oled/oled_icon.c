#include "oled_icon.h"
#include "ssd1306.h"
#include "oled.h"

/* ── WiFi icon bitmaps (column-major 8×8, bit 0 = top row) ────────────── */

static const uint8_t s_wifi_anim_frames[4][8] = {
    {0x00, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00}, /* dot only   */
    {0x00, 0x00, 0x00, 0x50, 0x50, 0x00, 0x00, 0x00}, /* inner arc  */
    {0x00, 0x08, 0x04, 0x54, 0x54, 0x04, 0x08, 0x00}, /* middle arc */
    {0x02, 0x09, 0x05, 0x55, 0x55, 0x05, 0x09, 0x02}, /* outer arc  */
};

static const uint8_t s_wifi_connected[8] = {0x02, 0x09, 0x05, 0x55, 0x55, 0x05, 0x09, 0x02};
static const uint8_t s_wifi_failed[8] = {0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81};

struct oled_context_t
{
    ssd1306_handle_t ssd1306;
};

void draw_bitmap_8x8(oled_handle_t handle, const uint8_t *bitmap, int xpos, int ypos)
{
    for (int col = 0; col < 8; col++)
    {
        for (int bit = 0; bit < 8; bit++)
        {
            if (bitmap[col] & (1 << bit))
            {
                ssd1306_set_pixel(handle->ssd1306, xpos + col, ypos + bit, false);
            }
        }
    }
}

void draw_wifi_icon(oled_handle_t handle, wifi_icon_state_t state, int xpos, int ypos)
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

void oled_render_home(oled_handle_t handle, const char *title,
                      wifi_icon_state_t wifi_state, const char *status_text)
{
    ssd1306_set_pages(handle->ssd1306, clear_screen_buffer);
    set_text(handle, title, 0, 0);
    draw_wifi_icon(handle, wifi_state, 120, 0);
    set_text(handle, status_text, 0, 18);
    ssd1306_display_pages(handle->ssd1306);
}
