/*
 * ESP32 Hardware Abstraction Layer - Display Implementation
 * Uses Adafruit_ST7789 library for TFT display control
 */

#include "hal/hal_display.h"
#include "hardware_config.h"

#ifdef PLATFORM_ESP32

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <stdarg.h>

// Global display instance
static Adafruit_ST7789* g_display = nullptr;
static bool g_initialized = false;
static hal_display_rotation_t g_rotation = HAL_DISPLAY_ROTATION_0;
static uint8_t g_backlight_brightness = 255;

// Display statistics
static uint32_t g_frames_rendered = 0;
static uint32_t g_pixels_drawn = 0;

// Text rendering state
static uint16_t g_text_color = HAL_COLOR_WHITE;
static uint16_t g_text_bg_color = HAL_COLOR_BLACK;
static hal_font_size_t g_text_size = HAL_FONT_SIZE_MEDIUM;
static int16_t g_cursor_x = 0;
static int16_t g_cursor_y = 0;

extern "C" {

// Display initialization and control
bool hal_display_init(void) {
    if (g_initialized) {
        return true;
    }
    
    // Initialize SPI for display
    SPI.begin();
    
    // Create display instance with hardware pins
    g_display = new Adafruit_ST7789(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);
    if (!g_display) {
        return false;
    }
    
    // Initialize the display
    g_display->init(HAL_DISPLAY_WIDTH, HAL_DISPLAY_HEIGHT);
    
    // Set initial rotation and clear screen
    g_display->setRotation(g_rotation);
    g_display->fillScreen(HAL_COLOR_BLACK);
    
    // Initialize backlight
    if (TFT_BL_PIN != -1) {
        pinMode(TFT_BL_PIN, OUTPUT);
        analogWrite(TFT_BL_PIN, g_backlight_brightness);
    }
    
    g_initialized = true;
    return true;
}

void hal_display_deinit(void) {
    if (!g_initialized) {
        return;
    }
    
    // Turn off backlight
    if (TFT_BL_PIN != -1) {
        digitalWrite(TFT_BL_PIN, LOW);
    }
    
    // Clean up display instance
    if (g_display) {
        delete g_display;
        g_display = nullptr;
    }
    
    g_initialized = false;
}

bool hal_display_is_initialized(void) {
    return g_initialized;
}

// Display properties
uint16_t hal_display_get_width(void) {
    if (!g_initialized || !g_display) return 0;
    return g_display->width();
}

uint16_t hal_display_get_height(void) {
    if (!g_initialized || !g_display) return 0;
    return g_display->height();
}

void hal_display_set_rotation(hal_display_rotation_t rotation) {
    if (!g_initialized || !g_display) return;
    
    g_rotation = rotation;
    g_display->setRotation((uint8_t)rotation);
}

hal_display_rotation_t hal_display_get_rotation(void) {
    return g_rotation;
}

// Backlight control
void hal_display_set_backlight(uint8_t brightness) {
    g_backlight_brightness = brightness;
    
    if (TFT_BL_PIN != -1) {
        analogWrite(TFT_BL_PIN, brightness);
    }
}

uint8_t hal_display_get_backlight(void) {
    return g_backlight_brightness;
}

// Basic drawing operations
void hal_display_clear(uint16_t color) {
    if (!g_initialized || !g_display) return;
    
    g_display->fillScreen(color);
    g_frames_rendered++;
}

void hal_display_fill_screen(uint16_t color) {
    hal_display_clear(color);
}

void hal_display_set_pixel(int16_t x, int16_t y, uint16_t color) {
    if (!g_initialized || !g_display) return;
    
    g_display->drawPixel(x, y, color);
    g_pixels_drawn++;
}

uint16_t hal_display_get_pixel(int16_t x, int16_t y) {
    // ST7789 doesn't support reading pixels, return black
    return HAL_COLOR_BLACK;
}

// Shape drawing
void hal_display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (!g_initialized || !g_display) return;
    
    g_display->drawLine(x0, y0, x1, y1, color);
    g_pixels_drawn += abs(x1 - x0) + abs(y1 - y0);
}

void hal_display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!g_initialized || !g_display) return;
    
    g_display->drawRect(x, y, w, h, color);
    g_pixels_drawn += (w + h) * 2;
}

void hal_display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!g_initialized || !g_display) return;
    
    g_display->fillRect(x, y, w, h, color);
    g_pixels_drawn += w * h;
}

void hal_display_draw_circle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    if (!g_initialized || !g_display) return;
    
    g_display->drawCircle(x, y, r, color);
    g_pixels_drawn += r * 6; // Approximate
}

void hal_display_fill_circle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    if (!g_initialized || !g_display) return;
    
    g_display->fillCircle(x, y, r, color);
    g_pixels_drawn += r * r * 3; // Approximate
}

void hal_display_draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    if (!g_initialized || !g_display) return;
    
    g_display->drawTriangle(x0, y0, x1, y1, x2, y2, color);
    g_pixels_drawn += abs(x1 - x0) + abs(y1 - y0) + abs(x2 - x1) + abs(y2 - y1) + abs(x0 - x2) + abs(y0 - y2);
}

void hal_display_fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    if (!g_initialized || !g_display) return;
    
    g_display->fillTriangle(x0, y0, x1, y1, x2, y2, color);
    // Approximate pixel count for filled triangle
    int16_t area = abs((x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)) / 2;
    g_pixels_drawn += area;
}

// Text rendering
void hal_display_set_text_color(uint16_t color) {
    g_text_color = color;
    if (g_initialized && g_display) {
        g_display->setTextColor(color);
    }
}

void hal_display_set_text_background(uint16_t color) {
    g_text_bg_color = color;
    if (g_initialized && g_display) {
        g_display->setTextColor(g_text_color, color);
    }
}

void hal_display_set_text_size(hal_font_size_t size) {
    g_text_size = size;
    if (g_initialized && g_display) {
        g_display->setTextSize((uint8_t)size);
    }
}

void hal_display_set_cursor(int16_t x, int16_t y) {
    g_cursor_x = x;
    g_cursor_y = y;
    if (g_initialized && g_display) {
        g_display->setCursor(x, y);
    }
}

void hal_display_print_char(char c) {
    if (!g_initialized || !g_display) return;
    
    g_display->print(c);
    g_pixels_drawn += 6 * 8 * g_text_size; // Approximate character size
}

void hal_display_print_string(const char* str) {
    if (!g_initialized || !g_display || !str) return;
    
    g_display->print(str);
    g_pixels_drawn += strlen(str) * 6 * 8 * g_text_size; // Approximate
}

void hal_display_printf(const char* format, ...) {
    if (!g_initialized || !g_display || !format) return;
    
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    hal_display_print_string(buffer);
}

// Advanced text rendering
void hal_display_draw_text(int16_t x, int16_t y, const char* text, uint16_t color, hal_font_size_t size) {
    if (!g_initialized || !g_display || !text) return;
    
    g_display->setCursor(x, y);
    g_display->setTextColor(color);
    g_display->setTextSize((uint8_t)size);
    g_display->print(text);
    
    g_pixels_drawn += strlen(text) * 6 * 8 * size;
}

void hal_display_draw_text_aligned(int16_t x, int16_t y, int16_t w, const char* text, 
                                   uint16_t color, hal_font_size_t size, hal_text_align_t align) {
    if (!g_initialized || !g_display || !text) return;
    
    int16_t text_width = strlen(text) * 6 * size;
    int16_t text_x = x;
    
    switch (align) {
        case HAL_TEXT_ALIGN_CENTER:
            text_x = x + (w - text_width) / 2;
            break;
        case HAL_TEXT_ALIGN_RIGHT:
            text_x = x + w - text_width;
            break;
        case HAL_TEXT_ALIGN_LEFT:
        default:
            text_x = x;
            break;
    }
    
    hal_display_draw_text(text_x, y, text, color, size);
}

// Bitmap/image operations
void hal_display_draw_bitmap(int16_t x, int16_t y, const uint8_t* bitmap, 
                            int16_t w, int16_t h, uint16_t color) {
    if (!g_initialized || !g_display || !bitmap) return;
    
    g_display->drawBitmap(x, y, bitmap, w, h, color);
    g_pixels_drawn += w * h;
}

void hal_display_draw_rgb_bitmap(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h) {
    if (!g_initialized || !g_display || !bitmap) return;
    
    g_display->drawRGBBitmap(x, y, bitmap, w, h);
    g_pixels_drawn += w * h;
}

// Buffer operations
void hal_display_start_write(void) {
    if (g_initialized && g_display) {
        g_display->startWrite();
    }
}

void hal_display_end_write(void) {
    if (g_initialized && g_display) {
        g_display->endWrite();
    }
}

void hal_display_write_pixel(int16_t x, int16_t y, uint16_t color) {
    if (g_initialized && g_display) {
        g_display->writePixel(x, y, color);
        g_pixels_drawn++;
    }
}

void hal_display_write_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (g_initialized && g_display) {
        g_display->writeFillRect(x, y, w, h, color);
        g_pixels_drawn += w * h;
    }
}

// Display refresh and synchronization
void hal_display_update(void) {
    // ST7789 updates immediately, no buffering
    g_frames_rendered++;
}

void hal_display_vsync(void) {
    // No hardware vsync available, just a small delay
    delay(16); // ~60 FPS
}

bool hal_display_is_busy(void) {
    // ST7789 is never busy in this implementation
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
    if (frames_rendered) *frames_rendered = g_frames_rendered;
    if (pixels_drawn) *pixels_drawn = g_pixels_drawn;
}

void hal_display_reset_stats(void) {
    g_frames_rendered = 0;
    g_pixels_drawn = 0;
}

} // extern "C"

#endif // PLATFORM_ESP32

