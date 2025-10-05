/*
 * Host Hardware Abstraction Layer - Display Implementation
 * Uses SDL2 for display emulation on PC
 */

#include "hal/hal_display.h"

#ifdef PLATFORM_HOST

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// SDL2 display state
static struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* framebuffer;
    TTF_Font* font;
    bool initialized;
    
    // Display properties
    hal_display_rotation_t rotation;
    uint8_t backlight_brightness;
    
    // Text rendering state
    uint16_t text_color;
    uint16_t text_bg_color;
    hal_font_size_t text_size;
    int16_t cursor_x;
    int16_t cursor_y;
    
    // Performance stats
    uint32_t frames_rendered;
    uint32_t pixels_drawn;
    
} g_sdl_display;

// Helper functions
static void sdl_set_draw_color(uint16_t color565);
static SDL_Color color565_to_sdl(uint16_t color565);
static void render_present(void);

extern "C" {

// Display initialization and control
bool hal_display_init(void) {
    if (g_sdl_display.initialized) {
        return true;
    }
    
    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL2 initialization failed: %s\n", SDL_GetError());
        return false;
    }
    
    // Initialize SDL2_ttf
    if (TTF_Init() < 0) {
        printf("SDL2_ttf initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return false;
    }
    
    // Create window
    g_sdl_display.window = SDL_CreateWindow(
        "Izod Mini Display Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        HAL_DISPLAY_WIDTH * 2,  // 2x scale for better visibility
        HAL_DISPLAY_HEIGHT * 2,
        SDL_WINDOW_SHOWN
    );
    
    if (!g_sdl_display.window) {
        printf("SDL2 window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return false;
    }
    
    // Create renderer
    g_sdl_display.renderer = SDL_CreateRenderer(
        g_sdl_display.window, 
        -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!g_sdl_display.renderer) {
        printf("SDL2 renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_sdl_display.window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }
    
    // Create framebuffer texture
    g_sdl_display.framebuffer = SDL_CreateTexture(
        g_sdl_display.renderer,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_TARGET,
        HAL_DISPLAY_WIDTH,
        HAL_DISPLAY_HEIGHT
    );
    
    if (!g_sdl_display.framebuffer) {
        printf("SDL2 framebuffer creation failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(g_sdl_display.renderer);
        SDL_DestroyWindow(g_sdl_display.window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }
    
    // Load default font (try to find a system font)
    const char* font_paths[] = {
        "/System/Library/Fonts/Helvetica.ttc",  // macOS
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",  // Linux
        "C:\\Windows\\Fonts\\arial.ttf",  // Windows
        "./assets/font.ttf"  // Local font
    };
    
    for (size_t i = 0; i < sizeof(font_paths) / sizeof(font_paths[0]); i++) {
        g_sdl_display.font = TTF_OpenFont(font_paths[i], 16);
        if (g_sdl_display.font) {
            break;
        }
    }
    
    if (!g_sdl_display.font) {
        printf("Warning: Could not load font, text rendering will be disabled\n");
    }
    
    // Initialize state
    g_sdl_display.rotation = HAL_DISPLAY_ROTATION_0;
    g_sdl_display.backlight_brightness = 255;
    g_sdl_display.text_color = HAL_COLOR_WHITE;
    g_sdl_display.text_bg_color = HAL_COLOR_BLACK;
    g_sdl_display.text_size = HAL_FONT_SIZE_MEDIUM;
    g_sdl_display.cursor_x = 0;
    g_sdl_display.cursor_y = 0;
    g_sdl_display.frames_rendered = 0;
    g_sdl_display.pixels_drawn = 0;
    
    // Clear screen to black
    SDL_SetRenderTarget(g_sdl_display.renderer, g_sdl_display.framebuffer);
    SDL_SetRenderDrawColor(g_sdl_display.renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_sdl_display.renderer);
    render_present();
    
    g_sdl_display.initialized = true;
    printf("SDL2 display emulator initialized (%dx%d)\n", HAL_DISPLAY_WIDTH, HAL_DISPLAY_HEIGHT);
    
    return true;
}

void hal_display_deinit(void) {
    if (!g_sdl_display.initialized) {
        return;
    }
    
    if (g_sdl_display.font) {
        TTF_CloseFont(g_sdl_display.font);
        g_sdl_display.font = nullptr;
    }
    
    if (g_sdl_display.framebuffer) {
        SDL_DestroyTexture(g_sdl_display.framebuffer);
        g_sdl_display.framebuffer = nullptr;
    }
    
    if (g_sdl_display.renderer) {
        SDL_DestroyRenderer(g_sdl_display.renderer);
        g_sdl_display.renderer = nullptr;
    }
    
    if (g_sdl_display.window) {
        SDL_DestroyWindow(g_sdl_display.window);
        g_sdl_display.window = nullptr;
    }
    
    TTF_Quit();
    SDL_Quit();
    
    g_sdl_display.initialized = false;
    printf("SDL2 display emulator deinitialized\n");
}

bool hal_display_is_initialized(void) {
    return g_sdl_display.initialized;
}

// Display properties
uint16_t hal_display_get_width(void) {
    if (!g_sdl_display.initialized) return 0;
    return HAL_DISPLAY_WIDTH;
}

uint16_t hal_display_get_height(void) {
    if (!g_sdl_display.initialized) return 0;
    return HAL_DISPLAY_HEIGHT;
}

void hal_display_set_rotation(hal_display_rotation_t rotation) {
    if (!g_sdl_display.initialized) return;
    
    g_sdl_display.rotation = rotation;
    // Note: SDL2 implementation doesn't actually rotate the display
    // This would require more complex coordinate transformation
}

hal_display_rotation_t hal_display_get_rotation(void) {
    return g_sdl_display.rotation;
}

// Backlight control
void hal_display_set_backlight(uint8_t brightness) {
    g_sdl_display.backlight_brightness = brightness;
    
    // Simulate backlight by adjusting window opacity or title
    if (g_sdl_display.window) {
        char title[256];
        snprintf(title, sizeof(title), "Izod Mini Display Emulator (Backlight: %d%%)", 
                 (brightness * 100) / 255);
        SDL_SetWindowTitle(g_sdl_display.window, title);
    }
}

uint8_t hal_display_get_backlight(void) {
    return g_sdl_display.backlight_brightness;
}

// Basic drawing operations
void hal_display_clear(uint16_t color) {
    if (!g_sdl_display.initialized) return;
    
    SDL_SetRenderTarget(g_sdl_display.renderer, g_sdl_display.framebuffer);
    sdl_set_draw_color(color);
    SDL_RenderClear(g_sdl_display.renderer);
    render_present();
    
    g_sdl_display.frames_rendered++;
    g_sdl_display.pixels_drawn += HAL_DISPLAY_WIDTH * HAL_DISPLAY_HEIGHT;
}

void hal_display_fill_screen(uint16_t color) {
    hal_display_clear(color);
}

void hal_display_set_pixel(int16_t x, int16_t y, uint16_t color) {
    if (!g_sdl_display.initialized) return;
    if (x < 0 || x >= HAL_DISPLAY_WIDTH || y < 0 || y >= HAL_DISPLAY_HEIGHT) return;
    
    SDL_SetRenderTarget(g_sdl_display.renderer, g_sdl_display.framebuffer);
    sdl_set_draw_color(color);
    SDL_RenderDrawPoint(g_sdl_display.renderer, x, y);
    
    g_sdl_display.pixels_drawn++;
}

uint16_t hal_display_get_pixel(int16_t x, int16_t y) {
    // SDL2 doesn't easily support reading pixels, return black
    return HAL_COLOR_BLACK;
}

// Shape drawing
void hal_display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (!g_sdl_display.initialized) return;
    
    SDL_SetRenderTarget(g_sdl_display.renderer, g_sdl_display.framebuffer);
    sdl_set_draw_color(color);
    SDL_RenderDrawLine(g_sdl_display.renderer, x0, y0, x1, y1);
    
    g_sdl_display.pixels_drawn += abs(x1 - x0) + abs(y1 - y0);
}

void hal_display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!g_sdl_display.initialized) return;
    
    SDL_Rect rect = {x, y, w, h};
    SDL_SetRenderTarget(g_sdl_display.renderer, g_sdl_display.framebuffer);
    sdl_set_draw_color(color);
    SDL_RenderDrawRect(g_sdl_display.renderer, &rect);
    
    g_sdl_display.pixels_drawn += (w + h) * 2;
}

void hal_display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!g_sdl_display.initialized) return;
    
    SDL_Rect rect = {x, y, w, h};
    SDL_SetRenderTarget(g_sdl_display.renderer, g_sdl_display.framebuffer);
    sdl_set_draw_color(color);
    SDL_RenderFillRect(g_sdl_display.renderer, &rect);
    
    g_sdl_display.pixels_drawn += w * h;
}

void hal_display_draw_circle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    if (!g_sdl_display.initialized) return;
    
    SDL_SetRenderTarget(g_sdl_display.renderer, g_sdl_display.framebuffer);
    sdl_set_draw_color(color);
    
    // Simple circle drawing algorithm
    for (int angle = 0; angle < 360; angle++) {
        int px = x + (int)(r * cos(angle * M_PI / 180.0));
        int py = y + (int)(r * sin(angle * M_PI / 180.0));
        SDL_RenderDrawPoint(g_sdl_display.renderer, px, py);
    }
    
    g_sdl_display.pixels_drawn += r * 6; // Approximate
}

void hal_display_fill_circle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    if (!g_sdl_display.initialized) return;
    
    SDL_SetRenderTarget(g_sdl_display.renderer, g_sdl_display.framebuffer);
    sdl_set_draw_color(color);
    
    // Fill circle by drawing filled rectangles
    for (int dy = -r; dy <= r; dy++) {
        int dx = (int)sqrt(r * r - dy * dy);
        SDL_Rect rect = {x - dx, y + dy, dx * 2, 1};
        SDL_RenderFillRect(g_sdl_display.renderer, &rect);
    }
    
    g_sdl_display.pixels_drawn += r * r * 3; // Approximate
}

void hal_display_draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    if (!g_sdl_display.initialized) return;
    
    hal_display_draw_line(x0, y0, x1, y1, color);
    hal_display_draw_line(x1, y1, x2, y2, color);
    hal_display_draw_line(x2, y2, x0, y0, color);
}

void hal_display_fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    if (!g_sdl_display.initialized) return;
    
    // Simple triangle fill - just fill bounding rectangle for now
    int16_t min_x = (x0 < x1) ? ((x0 < x2) ? x0 : x2) : ((x1 < x2) ? x1 : x2);
    int16_t min_y = (y0 < y1) ? ((y0 < y2) ? y0 : y2) : ((y1 < y2) ? y1 : y2);
    int16_t max_x = (x0 > x1) ? ((x0 > x2) ? x0 : x2) : ((x1 > x2) ? x1 : x2);
    int16_t max_y = (y0 > y1) ? ((y0 > y2) ? y0 : y2) : ((y1 > y2) ? y1 : y2);
    
    hal_display_fill_rect(min_x, min_y, max_x - min_x, max_y - min_y, color);
}

// Text rendering
void hal_display_set_text_color(uint16_t color) {
    g_sdl_display.text_color = color;
}

void hal_display_set_text_background(uint16_t color) {
    g_sdl_display.text_bg_color = color;
}

void hal_display_set_text_size(hal_font_size_t size) {
    g_sdl_display.text_size = size;
}

void hal_display_set_cursor(int16_t x, int16_t y) {
    g_sdl_display.cursor_x = x;
    g_sdl_display.cursor_y = y;
}

void hal_display_print_char(char c) {
    char str[2] = {c, '\0'};
    hal_display_print_string(str);
}

void hal_display_print_string(const char* str) {
    if (!g_sdl_display.initialized || !str || !g_sdl_display.font) return;
    
    SDL_Color text_color = color565_to_sdl(g_sdl_display.text_color);
    SDL_Surface* text_surface = TTF_RenderText_Solid(g_sdl_display.font, str, text_color);
    
    if (text_surface) {
        SDL_Texture* text_texture = SDL_CreateTextureFromSurface(g_sdl_display.renderer, text_surface);
        
        if (text_texture) {
            SDL_Rect dst_rect = {
                g_sdl_display.cursor_x,
                g_sdl_display.cursor_y,
                text_surface->w,
                text_surface->h
            };
            
            SDL_SetRenderTarget(g_sdl_display.renderer, g_sdl_display.framebuffer);
            SDL_RenderCopy(g_sdl_display.renderer, text_texture, nullptr, &dst_rect);
            
            // Update cursor position
            g_sdl_display.cursor_x += text_surface->w;
            
            SDL_DestroyTexture(text_texture);
            g_sdl_display.pixels_drawn += text_surface->w * text_surface->h;
        }
        
        SDL_FreeSurface(text_surface);
    }
}

void hal_display_printf(const char* format, ...) {
    if (!g_sdl_display.initialized || !format) return;
    
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    hal_display_print_string(buffer);
}

// Advanced text rendering
void hal_display_draw_text(int16_t x, int16_t y, const char* text, uint16_t color, hal_font_size_t size) {
    if (!g_sdl_display.initialized || !text) return;
    
    hal_display_set_cursor(x, y);
    hal_display_set_text_color(color);
    hal_display_set_text_size(size);
    hal_display_print_string(text);
}

void hal_display_draw_text_aligned(int16_t x, int16_t y, int16_t w, const char* text, 
                                   uint16_t color, hal_font_size_t size, hal_text_align_t align) {
    if (!g_sdl_display.initialized || !text || !g_sdl_display.font) return;
    
    int text_width, text_height;
    TTF_SizeText(g_sdl_display.font, text, &text_width, &text_height);
    
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
    if (!g_sdl_display.initialized || !bitmap) return;
    
    SDL_SetRenderTarget(g_sdl_display.renderer, g_sdl_display.framebuffer);
    sdl_set_draw_color(color);
    
    // Simple bitmap rendering - treat as 1-bit bitmap
    for (int16_t py = 0; py < h; py++) {
        for (int16_t px = 0; px < w; px++) {
            int byte_index = (py * ((w + 7) / 8)) + (px / 8);
            int bit_index = 7 - (px % 8);
            
            if (bitmap[byte_index] & (1 << bit_index)) {
                SDL_RenderDrawPoint(g_sdl_display.renderer, x + px, y + py);
            }
        }
    }
    
    g_sdl_display.pixels_drawn += w * h;
}

void hal_display_draw_rgb_bitmap(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h) {
    if (!g_sdl_display.initialized || !bitmap) return;
    
    SDL_SetRenderTarget(g_sdl_display.renderer, g_sdl_display.framebuffer);
    
    // Draw RGB565 bitmap pixel by pixel
    for (int16_t py = 0; py < h; py++) {
        for (int16_t px = 0; px < w; px++) {
            uint16_t color = bitmap[py * w + px];
            sdl_set_draw_color(color);
            SDL_RenderDrawPoint(g_sdl_display.renderer, x + px, y + py);
        }
    }
    
    g_sdl_display.pixels_drawn += w * h;
}

// Buffer operations
void hal_display_start_write(void) {
    if (g_sdl_display.initialized) {
        SDL_SetRenderTarget(g_sdl_display.renderer, g_sdl_display.framebuffer);
    }
}

void hal_display_end_write(void) {
    // No special handling needed for SDL2
}

void hal_display_write_pixel(int16_t x, int16_t y, uint16_t color) {
    hal_display_set_pixel(x, y, color);
}

void hal_display_write_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    hal_display_fill_rect(x, y, w, h, color);
}

// Display refresh and synchronization
void hal_display_update(void) {
    if (g_sdl_display.initialized) {
        render_present();
        g_sdl_display.frames_rendered++;
    }
}

void hal_display_vsync(void) {
    // SDL2 renderer already handles vsync if enabled
    SDL_Delay(16); // ~60 FPS
}

bool hal_display_is_busy(void) {
    return false; // Never busy in SDL2 implementation
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
    if (frames_rendered) *frames_rendered = g_sdl_display.frames_rendered;
    if (pixels_drawn) *pixels_drawn = g_sdl_display.pixels_drawn;
}

void hal_display_reset_stats(void) {
    g_sdl_display.frames_rendered = 0;
    g_sdl_display.pixels_drawn = 0;
}

} // extern "C"

// Helper functions
static void sdl_set_draw_color(uint16_t color565) {
    uint8_t r, g, b;
    hal_display_color565_to_rgb(color565, &r, &g, &b);
    SDL_SetRenderDrawColor(g_sdl_display.renderer, r, g, b, 255);
}

static SDL_Color color565_to_sdl(uint16_t color565) {
    SDL_Color color;
    hal_display_color565_to_rgb(color565, &color.r, &color.g, &color.b);
    color.a = 255;
    return color;
}

static void render_present(void) {
    if (!g_sdl_display.initialized) return;
    
    // Copy framebuffer to window with 2x scaling
    SDL_SetRenderTarget(g_sdl_display.renderer, nullptr);
    SDL_RenderCopy(g_sdl_display.renderer, g_sdl_display.framebuffer, nullptr, nullptr);
    SDL_RenderPresent(g_sdl_display.renderer);
    
    // Process SDL events to keep window responsive
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            // Handle quit event gracefully
            printf("SDL2 quit event received\n");
        }
    }
}

#endif // PLATFORM_HOST

