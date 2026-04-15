#ifndef OLED_H
#define OLED_H

extern const uint8_t zero_buf[4 * 128];
extern uint8_t *clear_screen_buffer;

typedef enum {
    WIFI_ICON_CONNECTING = 0,
    WIFI_ICON_CONNECTED,
    WIFI_ICON_FAILED,
} wifi_icon_state_t;

void set_latin_text(ssd1306_handle_t handle, const char *text, int xpos, int ypos);
void draw_bitmap_8x8(ssd1306_handle_t handle, const uint8_t *bitmap, int xpos, int ypos);
void draw_wifi_icon(ssd1306_handle_t handle, wifi_icon_state_t state, int xpos, int ypos);
void oled_render_home(ssd1306_handle_t handle, const char *title, wifi_icon_state_t wifi_state, const char *status_text, int led_brightness);
esp_err_t brightness_control(ssd1306_handle_t handle, int value, int timeout_seconds);
ssd1306_handle_t setup_screen(void);

#endif // OLED_H
