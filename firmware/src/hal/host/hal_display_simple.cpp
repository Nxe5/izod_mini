/*
 * Simple Host Display HAL Implementation (No SDL2)
 * For basic testing without graphics dependencies
 */

#include "hal/hal_display.h"

#ifdef PLATFORM_HOST

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

// Simple display state
static struct {
    bool initialized;
    hal_display_rotation_t rotation;
    uint8_t backlight_brightness;
    uint16_t text_color;
    uint16_t text_bg_color;
    hal_font_size_t text_size;
    int16_t cursor_x;
    int16_t cursor_y;
    uint32_t frames_rendered;
    uint32_t pixels_drawn;
} g_simple_display;

extern "C" {

// Display initialization and control
bool hal_display_init(void) {
    if (g_simple_display.initialized) {
        return true;
    }
    
    g_simple_display.rotation = HAL_DISPLAY_ROTATION_0;
    g_simple_display.backlight_brightness = 255;
    g_simple_display.text_color = HAL_COLOR_WHITE;
    g_simple_display.text_bg_color = HAL_COLOR_BLACK;
    g_simple_display.text_size = HAL_FONT_SIZE_MEDIUM;
    g_simple_display.cursor_x = 0;
    g_simple_display.cursor_y = 0;
    g_simple_display.frames_rendered = 0;
    g_simple_display.pixels_drawn = 0;
    
    g_simple_display.initialized = true;
    printf("Simple display HAL initialized (%dx%d)\n", HAL_DISPLAY_WIDTH, HAL_DISPLAY_HEIGHT);
    
    return true;
}

void hal_display_deinit(void) {
    g_simple_display.initialized = false;
    printf("Simple display HAL deinitialized\n");
}

bool hal_display_is_initialized(void) {
    return g_simple_display.initialized;
}

// Display properties
uint16_t hal_display_get_width(void) {
    return HAL_DISPLAY_WIDTH;
}

uint16_t hal_display_get_height(void) {
    return HAL_DISPLAY_HEIGHT;
}

void hal_display_set_rotation(hal_display_rotation_t rotation) {
    g_simple_display.rotation = rotation;
}

hal_display_rotation_t hal_display_get_rotation(void) {
    return g_simple_display.rotation;
}

// Backlight control
void hal_display_set_backlight(uint8_t brightness) {
    g_simple_display.backlight_brightness = brightness;
    printf("Display backlight set to %d%%\n", (brightness * 100) / 255);
}

uint8_t hal_display_get_backlight(void) {
    return g_simple_display.backlight_brightness;
}

// Basic drawing operations
void hal_display_clear(uint16_t color) {
    if (!g_simple_display.initialized) return;
    
    printf("Display cleared with color 0x%04X\n", color);
    g_simple_display.frames_rendered++;
    g_simple_display.pixels_drawn += HAL_DISPLAY_WIDTH * HAL_DISPLAY_HEIGHT;
}

void hal_display_fill_screen(uint16_t color) {
    hal_display_clear(color);
}

void hal_display_set_pixel(int16_t x, int16_t y, uint16_t color) {
    if (!g_simple_display.initialized) return;
    if (x < 0 || x >= HAL_DISPLAY_WIDTH || y < 0 || y >= HAL_DISPLAY_HEIGHT) return;
    
    printf("Pixel set at (%d,%d) with color 0x%04X\n", x, y, color);
    g_simple_display.pixels_drawn++;
}

uint16_t hal_display_get_pixel(int16_t x, int16_t y) {
    return HAL_COLOR_BLACK;
}

// Shape drawing
void hal_display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (!g_simple_display.initialized) return;
    
    printf("Line drawn from (%d,%d) to (%d,%d) with color 0x%04X\n", x0, y0, x1, y1, color);
    g_simple_display.pixels_drawn += abs(x1 - x0) + abs(y1 - y0);
}

void hal_display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!g_simple_display.initialized) return;
    
    printf("Rectangle drawn at (%d,%d) size %dx%d with color 0x%04X\n", x, y, w, h, color);
    g_simple_display.pixels_drawn += (w + h) * 2;
}

void hal_display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!g_simple_display.initialized) return;
    
    printf("Rectangle filled at (%d,%d) size %dx%d with color 0x%04X\n", x, y, w, h, color);
    g_simple_display.pixels_drawn += w * h;
}

void hal_display_draw_circle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    if (!g_simple_display.initialized) return;
    
    printf("Circle drawn at (%d,%d) radius %d with color 0x%04X\n", x, y, r, color);
    g_simple_display.pixels_drawn += r * 6;
}

void hal_display_fill_circle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    if (!g_simple_display.initialized) return;
    
    printf("Circle filled at (%d,%d) radius %d with color 0x%04X\n", x, y, r, color);
    g_simple_display.pixels_drawn += r * r * 3;
}

void hal_display_draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    if (!g_simple_display.initialized) return;
    
    printf("Triangle drawn at (%d,%d) (%d,%d) (%d,%d) with color 0x%04X\n", x0, y0, x1, y1, x2, y2, color);
    g_simple_display.pixels_drawn += 100; // Approximate
}

void hal_display_fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    if (!g_simple_display.initialized) return;
    
    printf("Triangle filled at (%d,%d) (%d,%d) (%d,%d) with color 0x%04X\n", x0, y0, x1, y1, x2, y2, color);
    g_simple_display.pixels_drawn += 200; // Approximate
}

// Text rendering
void hal_display_set_text_color(uint16_t color) {
    g_simple_display.text_color = color;
}

void hal_display_set_text_background(uint16_t color) {
    g_simple_display.text_bg_color = color;
}

void hal_display_set_text_size(hal_font_size_t size) {
    g_simple_display.text_size = size;
}

void hal_display_set_cursor(int16_t x, int16_t y) {
    g_simple_display.cursor_x = x;
    g_simple_display.cursor_y = y;
}

void hal_display_print_char(char c) {
    if (!g_simple_display.initialized) return;
    
    printf("%c", c);
    g_simple_display.pixels_drawn += 6 * 8 * g_simple_display.text_size;
}

void hal_display_print_string(const char* str) {
    if (!g_simple_display.initialized || !str) return;
    
    printf("%s", str);
    g_simple_display.pixels_drawn += strlen(str) * 6 * 8 * g_simple_display.text_size;
}

void hal_display_printf(const char* format, ...) {
    if (!g_simple_display.initialized || !format) return;
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

// Advanced text rendering
void hal_display_draw_text(int16_t x, int16_t y, const char* text, uint16_t color, hal_font_size_t size) {
    if (!g_simple_display.initialized || !text) return;
    
    printf("Text at (%d,%d): %s (color: 0x%04X, size: %d)\n", x, y, text, color, size);
    g_simple_display.pixels_drawn += strlen(text) * 6 * 8 * size;
}

void hal_display_draw_text_aligned(int16_t x, int16_t y, int16_t w, const char* text, 
                                   uint16_t color, hal_font_size_t size, hal_text_align_t align) {
    if (!g_simple_display.initialized || !text) return;
    
    printf("Text aligned at (%d,%d) width %d: %s (color: 0x%04X, size: %d, align: %d)\n", 
           x, y, w, text, color, size, align);
    g_simple_display.pixels_drawn += strlen(text) * 6 * 8 * size;
}

// Bitmap/image operations
void hal_display_draw_bitmap(int16_t x, int16_t y, const uint8_t* bitmap, 
                            int16_t w, int16_t h, uint16_t color) {
    if (!g_simple_display.initialized || !bitmap) return;
    
    printf("Bitmap drawn at (%d,%d) size %dx%d with color 0x%04X\n", x, y, w, h, color);
    g_simple_display.pixels_drawn += w * h;
}

void hal_display_draw_rgb_bitmap(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h) {
    if (!g_simple_display.initialized || !bitmap) return;
    
    printf("RGB bitmap drawn at (%d,%d) size %dx%d\n", x, y, w, h);
    g_simple_display.pixels_drawn += w * h;
}

// Buffer operations
void hal_display_start_write(void) {
    // No-op in simple implementation
}

void hal_display_end_write(void) {
    // No-op in simple implementation
}

void hal_display_write_pixel(int16_t x, int16_t y, uint16_t color) {
    hal_display_set_pixel(x, y, color);
}

void hal_display_write_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    hal_display_fill_rect(x, y, w, h, color);
}

// Display refresh and synchronization
void hal_display_update(void) {
    if (g_simple_display.initialized) {
        g_simple_display.frames_rendered++;
    }
}

void hal_display_vsync(void) {
    // Simple delay
    // usleep(16000); // ~60 FPS
}

bool hal_display_is_busy(void) {
    return false;
}

// Color utilities
uint16_t hal_display_color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void hal_display_color565_to_rgb(uint16_t color, uint8_t* r, uint8_t* g, uint8_t* b) {
    if (r) *r = (color >> 8) & 0xF8;
    if (g) *g = (color >> 3) & 0xFC;
    if (b) *b = (color << 3) & 0xF8;
}

// Performance and debugging
void hal_display_get_stats(uint32_t* frames_rendered, uint32_t* pixels_drawn) {
    if (frames_rendered) *frames_rendered = g_simple_display.frames_rendered;
    if (pixels_drawn) *pixels_drawn = g_simple_display.pixels_drawn;
}

void hal_display_reset_stats(void) {
    g_simple_display.frames_rendered = 0;
    g_simple_display.pixels_drawn = 0;
}

} // extern "C"

#endif // PLATFORM_HOST
