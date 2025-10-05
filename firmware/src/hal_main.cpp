/*
 * HAL-based Main Application
 * Demonstrates the Hardware Abstraction Layer usage
 * Works on both ESP32 hardware and host emulation
 */

#include "hal/hal_display.h"
#include "hal/hal_system.h"
#include <stdio.h>
#include <math.h>

#ifdef PLATFORM_ESP32
#include <Arduino.h>
#endif

// Application state
static bool g_running = true;
static uint32_t g_frame_count = 0;

// Demo functions
static void demo_display_test(void);
static void demo_system_info(void);
static void demo_graphics_test(void);

#ifdef PLATFORM_ESP32
void setup() {
    Serial.begin(115200);
    Serial.println("Starting HAL Demo on ESP32...");
    
    // Initialize HAL
    if (!hal_system_init()) {
        Serial.println("Failed to initialize system HAL");
        return;
    }
    
    if (!hal_display_init()) {
        Serial.println("Failed to initialize display HAL");
        return;
    }
    
    Serial.println("HAL initialized successfully");
    
    // Run demos
    demo_system_info();
    demo_display_test();
}

void loop() {
    demo_graphics_test();
    hal_system_delay_ms(100);
}

#else // PLATFORM_HOST

int main() {
    printf("Starting HAL Demo on Host...\n");
    
    // Initialize HAL
    if (!hal_system_init()) {
        printf("Failed to initialize system HAL\n");
        return -1;
    }
    
    if (!hal_display_init()) {
        printf("Failed to initialize display HAL\n");
        return -1;
    }
    
    printf("HAL initialized successfully\n");
    
    // Run demos
    demo_system_info();
    demo_display_test();
    
    // Main loop
    while (g_running && g_frame_count < 1000) { // Limit frames for host demo
        demo_graphics_test();
        hal_system_delay_ms(100);
        
        // Check for exit condition (simplified)
        if (g_frame_count > 500) {
            g_running = false;
        }
    }
    
    printf("Demo completed, cleaning up...\n");
    
    // Cleanup
    hal_display_deinit();
    hal_system_deinit();
    
    return 0;
}

#endif

// Demo implementations
static void demo_system_info(void) {
    hal_system_info_t info;
    if (hal_system_get_info(&info)) {
        hal_system_log(HAL_LOG_LEVEL_INFO, "DEMO", "=== System Information ===");
        hal_system_log(HAL_LOG_LEVEL_INFO, "DEMO", "Chip: %s rev %lu", 
                       info.chip_model, info.chip_revision);
        hal_system_log(HAL_LOG_LEVEL_INFO, "DEMO", "CPU: %lu MHz", info.cpu_freq_mhz);
        hal_system_log(HAL_LOG_LEVEL_INFO, "DEMO", "Memory: %lu/%lu bytes free", 
                       info.free_heap, info.total_heap);
        hal_system_log(HAL_LOG_LEVEL_INFO, "DEMO", "Firmware: %s", info.firmware_version);
        hal_system_log(HAL_LOG_LEVEL_INFO, "DEMO", "Build: %s %s", info.build_date, info.build_time);
    }
}

static void demo_display_test(void) {
    hal_system_log(HAL_LOG_LEVEL_INFO, "DEMO", "Running display test...");
    
    // Clear screen
    hal_display_clear(HAL_COLOR_BLACK);
    
    // Display basic info
    hal_display_set_text_color(HAL_COLOR_WHITE);
    hal_display_set_text_size(HAL_FONT_SIZE_MEDIUM);
    hal_display_set_cursor(10, 10);
    hal_display_printf("Izod Mini HAL Demo");
    
    hal_display_set_cursor(10, 30);
    hal_display_printf("Display: %dx%d", hal_display_get_width(), hal_display_get_height());
    
    hal_display_set_cursor(10, 50);
    hal_display_printf("Platform: %s", hal_system_get_chip_model());
    
    // Draw some basic shapes
    hal_display_draw_rect(10, 80, 100, 60, HAL_COLOR_RED);
    hal_display_fill_rect(15, 85, 90, 50, HAL_COLOR_BLUE);
    
    hal_display_draw_circle(160, 110, 30, HAL_COLOR_GREEN);
    hal_display_fill_circle(160, 110, 20, HAL_COLOR_YELLOW);
    
    // Draw a line pattern
    for (int i = 0; i < 10; i++) {
        hal_display_draw_line(10 + i * 20, 160, 10 + i * 20, 200, HAL_COLOR_CYAN);
    }
    
    hal_display_update();
    hal_system_delay_ms(2000);
}

static void demo_graphics_test(void) {
    static uint32_t last_update = 0;
    uint32_t now = hal_system_get_time_ms();
    
    if (now - last_update < 50) return; // 20 FPS
    last_update = now;
    
    // Animated graphics demo
    hal_display_clear(HAL_COLOR_BLACK);
    
    // Title
    hal_display_set_text_color(HAL_COLOR_WHITE);
    hal_display_set_text_size(HAL_FONT_SIZE_LARGE);
    hal_display_draw_text_aligned(0, 10, HAL_DISPLAY_WIDTH, "HAL Demo", 
                                  HAL_COLOR_WHITE, HAL_FONT_SIZE_LARGE, HAL_TEXT_ALIGN_CENTER);
    
    // Frame counter
    hal_display_set_text_size(HAL_FONT_SIZE_SMALL);
    hal_display_set_cursor(10, HAL_DISPLAY_HEIGHT - 20);
    hal_display_printf("Frame: %lu", g_frame_count);
    
    // System stats
    hal_display_set_cursor(10, HAL_DISPLAY_HEIGHT - 40);
    hal_display_printf("Uptime: %lu ms", hal_system_get_uptime_ms());
    
    hal_display_set_cursor(10, HAL_DISPLAY_HEIGHT - 60);
    hal_display_printf("Free RAM: %lu bytes", hal_system_get_free_heap());
    
    // Animated elements
    int center_x = HAL_DISPLAY_WIDTH / 2;
    int center_y = HAL_DISPLAY_HEIGHT / 2;
    
    // Rotating circle
    float angle = (g_frame_count * 0.1f);
    int orbit_x = center_x + (int)(50 * cos(angle));
    int orbit_y = center_y + (int)(50 * sin(angle));
    hal_display_fill_circle(orbit_x, orbit_y, 10, HAL_COLOR_RED);
    
    // Pulsing rectangle
    int pulse_size = 20 + (int)(10 * sin(g_frame_count * 0.2f));
    hal_display_draw_rect(center_x - pulse_size/2, center_y - pulse_size/2, 
                         pulse_size, pulse_size, HAL_COLOR_GREEN);
    
    // Color cycling line
    uint16_t line_color = hal_display_color565(
        (uint8_t)(128 + 127 * sin(g_frame_count * 0.05f)),
        (uint8_t)(128 + 127 * sin(g_frame_count * 0.07f)),
        (uint8_t)(128 + 127 * sin(g_frame_count * 0.09f))
    );
    
    hal_display_draw_line(0, center_y, HAL_DISPLAY_WIDTH, center_y, line_color);
    hal_display_draw_line(center_x, 0, center_x, HAL_DISPLAY_HEIGHT, line_color);
    
    // Performance indicator
    uint32_t frames, pixels;
    hal_display_get_stats(&frames, &pixels);
    
    hal_display_set_text_color(HAL_COLOR_YELLOW);
    hal_display_set_cursor(HAL_DISPLAY_WIDTH - 100, 10);
    hal_display_printf("FPS: ~20");
    
    hal_display_set_cursor(HAL_DISPLAY_WIDTH - 100, 30);
    hal_display_printf("Frames: %lu", frames);
    
    hal_display_update();
    g_frame_count++;
    
    // Log periodic status
    if (g_frame_count % 100 == 0) {
        hal_system_log(HAL_LOG_LEVEL_INFO, "DEMO", "Frame %lu, uptime %lu ms", 
                       g_frame_count, hal_system_get_uptime_ms());
    }
}

