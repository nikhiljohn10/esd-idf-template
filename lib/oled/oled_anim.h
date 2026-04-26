#ifndef OLED_ANIM_H
#define OLED_ANIM_H

/**
 * @file oled_anim.h
 * @brief Animation helpers for the SSD1306 OLED display.
 *
 * This is an internal sub-module of the oled library.  It is included
 * automatically via oled.h — application code does not need to include
 * this header directly.
 *
 * Provided:
 *  - Animation helpers and stateful playback
 *  - Drawing wrappers around ssd1306 primitives (oled_set_pixel, oled_draw_line,
 *    oled_draw_rect, oled_fill_rect, oled_draw_circle, oled_fill_circle,
 *    oled_draw_bitmap, oled_flush)
 */

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

/* oled_handle_t is declared in oled.h, which always includes this file
 * after emitting the typedef.  When oled_anim.h is included on its own
 * the guard below provides a self-contained forward declaration so the
 * compiler never sees a conflicting definition. */
#ifndef OLED_H
struct oled_context_t;
typedef struct oled_context_t *oled_handle_t;
#endif

/* ── Drawing wrappers ───────────────────────────────────────────────────── */

/**
 * @brief Set a single pixel in the display buffer (no hardware flush).
 *
 * Call oled_flush() afterwards to make the change visible.
 *
 * @param handle Display handle.
 * @param x      Column (0 = left).
 * @param y      Row    (0 = top).
 * @param invert true = clear pixel; false = set pixel.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t oled_set_pixel(oled_handle_t handle, uint8_t x, uint8_t y, bool invert);

/**
 * @brief Draw a line and push to hardware immediately.
 *
 * @param handle Display handle.
 * @param x0     Start column.
 * @param y0     Start row.
 * @param x1     End column.
 * @param y1     End row.
 * @param invert true = clear pixels along line; false = set.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t oled_draw_line(oled_handle_t handle, uint8_t x0, uint8_t y0,
                         uint8_t x1, uint8_t y1, bool invert);

/**
 * @brief Draw a rectangle outline and push to hardware immediately.
 *
 * @param handle Display handle.
 * @param x      Left edge column.
 * @param y      Top edge row.
 * @param w      Width in pixels.
 * @param h      Height in pixels.
 * @param invert true = clear; false = set.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t oled_draw_rect(oled_handle_t handle, uint8_t x, uint8_t y,
                         uint8_t w, uint8_t h, bool invert);

/**
 * @brief Draw a filled rectangle and push to hardware immediately.
 *
 * @param handle Display handle.
 * @param x      Left edge column.
 * @param y      Top edge row.
 * @param w      Width in pixels.
 * @param h      Height in pixels.
 * @param invert true = clear; false = set.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t oled_fill_rect(oled_handle_t handle, uint8_t x, uint8_t y,
                         uint8_t w, uint8_t h, bool invert);

/**
 * @brief Draw a circle outline and push to hardware immediately.
 *
 * @param handle Display handle.
 * @param x0     Centre column.
 * @param y0     Centre row.
 * @param r      Radius in pixels.
 * @param invert true = clear; false = set.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t oled_draw_circle(oled_handle_t handle, uint8_t x0, uint8_t y0,
                           uint8_t r, bool invert);

/**
 * @brief Draw a filled circle and push to hardware immediately.
 *
 * @param handle Display handle.
 * @param x0     Centre column.
 * @param y0     Centre row.
 * @param r      Radius in pixels.
 * @param invert true = clear; false = set.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t oled_fill_circle(oled_handle_t handle, uint8_t x0, uint8_t y0,
                           uint8_t r, bool invert);

/**
 * @brief Draw an arbitrary bitmap and push to hardware immediately.
 *
 * The bitmap is row-major: each row is @p w bits, stored MSB-first in
 * ceil(w/8) bytes.  Successive rows follow without padding.
 *
 * @param handle Display handle.
 * @param x      Left edge column.
 * @param y      Top edge row.
 * @param bitmap Pointer to bitmap data.
 * @param w      Bitmap width in pixels.
 * @param h      Bitmap height in pixels.
 * @param invert true = invert each pixel; false = normal.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t oled_draw_bitmap(oled_handle_t handle, uint8_t x, uint8_t y,
                           const uint8_t *bitmap, uint8_t w, uint8_t h,
                           bool invert);

/**
 * @brief Flush the display buffer to hardware.
 *
 * Use after one or more oled_set_pixel() calls, or any time you want to
 * push pending buffer changes without drawing a new shape.
 *
 * @param handle Display handle.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t oled_flush(oled_handle_t handle);

/* ── Multi-frame animation patterns ─────────────────────────────────────── */

/** @brief Flash all pixels ON for hold_ms milliseconds. */
void oled_anim_all_on(oled_handle_t handle, uint32_t hold_ms);

/** @brief Animate a growing checkerboard pattern (1 → 16 px cell size). */
void oled_anim_checkerboard(oled_handle_t handle, uint32_t hold_ms);

/** @brief Animate growing vertical stripes (1 → 16 px wide). */
void oled_anim_vertical_stripes(oled_handle_t handle, uint32_t hold_ms);

/** @brief Animate growing horizontal stripes (1 → 16 px tall). */
void oled_anim_horizontal_stripes(oled_handle_t handle, uint32_t hold_ms);

/** @brief Animate shrinking concentric rectangles (gap grows 1 → 15). */
void oled_anim_concentric_rects(oled_handle_t handle, uint32_t hold_ms);

#endif /* OLED_ANIM_H */
