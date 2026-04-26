#ifndef OLED_ICON_H
#define OLED_ICON_H

#include <stdint.h>

/* oled_handle_t is declared in oled.h, which always includes this file
 * after emitting the typedef.  When oled_icon.h is included on its own
 * the guard below provides a self-contained forward declaration.
 */
#ifndef OLED_H
struct oled_context_t;
typedef struct oled_context_t *oled_handle_t;
#endif

/**
 * @brief State of the WiFi icon drawn by draw_wifi_icon().
 */
typedef enum
{
    WIFI_ICON_CONNECTING = 0, /*!< Animated: building up signal arcs */
    WIFI_ICON_CONNECTED,      /*!< Static: full signal icon */
    WIFI_ICON_FAILED,         /*!< Static: X / no-signal icon */
} wifi_icon_state_t;

/**
 * @brief Blit an 8×8 column-major bitmap onto the display buffer.
 */
void draw_bitmap_8x8(oled_handle_t handle, const uint8_t *bitmap, int xpos, int ypos);

/**
 * @brief Draw an animated or static WiFi signal icon into the display buffer.
 */
void draw_wifi_icon(oled_handle_t handle, wifi_icon_state_t state, int xpos, int ypos);

/**
 * @brief Render the standard home screen in a single call.
 *
 * Clears the buffer, renders all elements, then pushes to hardware.
 */
void oled_render_home(oled_handle_t handle, const char *title,
                      wifi_icon_state_t wifi_state, const char *status_text);

#endif /* OLED_ICON_H */
