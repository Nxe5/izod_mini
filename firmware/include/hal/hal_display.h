/*
 * Hardware Abstraction Layer - Display Interface
 * Abstracts TFT display operations for both ESP32 hardware and host emulation
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Display configuration constants
#define HAL_DISPLAY_WIDTH  240
#define HAL_DISPLAY_HEIGHT 320

// Color definitions (16-bit RGB565)
#define HAL_COLOR_BLACK   0x0000
#define HAL_COLOR_WHITE   0xFFFF
#define HAL_COLOR_RED     0xF800
#define HAL_COLOR_GREEN   0x07E0
#define HAL_COLOR_BLUE    0x001F
#define HAL_COLOR_YELLOW  0xFFE0
#define HAL_COLOR_CYAN    0x07FF
#define HAL_COLOR_MAGENTA 0xF81F
#define HAL_COLOR_GRAY    0x8410

// Display rotation modes
typedef enum {
    HAL_DISPLAY_ROTATION_0   = 0,  // Portrait
    HAL_DISPLAY_ROTATION_90  = 1,  // Landscape
    HAL_DISPLAY_ROTATION_180 = 2,  // Portrait (flipped)
    HAL_DISPLAY_ROTATION_270 = 3   // Landscape (flipped)
} hal_display_rotation_t;

// Text alignment
typedef enum {
    HAL_TEXT_ALIGN_LEFT   = 0,
    HAL_TEXT_ALIGN_CENTER = 1,
    HAL_TEXT_ALIGN_RIGHT  = 2
} hal_text_align_t;

// Font sizes
typedef enum {
    HAL_FONT_SIZE_SMALL  = 1,
    HAL_FONT_SIZE_MEDIUM = 2,
    HAL_FONT_SIZE_LARGE  = 3
} hal_font_size_t;

// Display initialization and control
bool hal_display_init(void);
void hal_display_deinit(void);
bool hal_display_is_initialized(void);

// Display properties
uint16_t hal_display_get_width(void);
uint16_t hal_display_get_height(void);
void hal_display_set_rotation(hal_display_rotation_t rotation);
hal_display_rotation_t hal_display_get_rotation(void);

// Backlight control
void hal_display_set_backlight(uint8_t brightness);  // 0-255
uint8_t hal_display_get_backlight(void);

// Basic drawing operations
void hal_display_clear(uint16_t color);
void hal_display_fill_screen(uint16_t color);
void hal_display_set_pixel(int16_t x, int16_t y, uint16_t color);
uint16_t hal_display_get_pixel(int16_t x, int16_t y);

// Shape drawing
void hal_display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void hal_display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void hal_display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void hal_display_draw_circle(int16_t x, int16_t y, int16_t r, uint16_t color);
void hal_display_fill_circle(int16_t x, int16_t y, int16_t r, uint16_t color);
void hal_display_draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void hal_display_fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

// Text rendering
void hal_display_set_text_color(uint16_t color);
void hal_display_set_text_background(uint16_t color);
void hal_display_set_text_size(hal_font_size_t size);
void hal_display_set_cursor(int16_t x, int16_t y);
void hal_display_print_char(char c);
void hal_display_print_string(const char* str);
void hal_display_printf(const char* format, ...);

// Advanced text rendering
void hal_display_draw_text(int16_t x, int16_t y, const char* text, uint16_t color, hal_font_size_t size);
void hal_display_draw_text_aligned(int16_t x, int16_t y, int16_t w, const char* text, 
                                   uint16_t color, hal_font_size_t size, hal_text_align_t align);

// Bitmap/image operations
void hal_display_draw_bitmap(int16_t x, int16_t y, const uint8_t* bitmap, 
                            int16_t w, int16_t h, uint16_t color);
void hal_display_draw_rgb_bitmap(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h);

// Buffer operations (for efficient updates)
void hal_display_start_write(void);
void hal_display_end_write(void);
void hal_display_write_pixel(int16_t x, int16_t y, uint16_t color);
void hal_display_write_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

// Display refresh and synchronization
void hal_display_update(void);          // Push changes to display
void hal_display_vsync(void);           // Wait for vertical sync
bool hal_display_is_busy(void);         // Check if display is busy

// Color utilities
uint16_t hal_display_color565(uint8_t r, uint8_t g, uint8_t b);
void hal_display_color565_to_rgb(uint16_t color, uint8_t* r, uint8_t* g, uint8_t* b);

// Performance and debugging
void hal_display_get_stats(uint32_t* frames_rendered, uint32_t* pixels_drawn);
void hal_display_reset_stats(void);

#ifdef __cplusplus
}
#endif

