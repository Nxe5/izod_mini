/*
 * Simple Test for HAL System
 * Basic functionality verification
 */

#include <unity.h>
#include "hal/hal_display.h"
#include "hal/hal_system.h"

void setUp(void) {
    // Set up test environment
}

void tearDown(void) {
    // Clean up test environment
}

void test_hal_display_init(void) {
    // Test display initialization
    bool result = hal_display_init();
    TEST_ASSERT_TRUE(result);
    
    // Test that display is initialized
    TEST_ASSERT_TRUE(hal_display_is_initialized());
    
    // Test display dimensions
    TEST_ASSERT_EQUAL(240, hal_display_get_width());
    TEST_ASSERT_EQUAL(320, hal_display_get_height());
    
    // Clean up
    hal_display_deinit();
}

void test_hal_display_basic_operations(void) {
    // Initialize display
    TEST_ASSERT_TRUE(hal_display_init());
    
    // Test basic operations
    hal_display_clear(HAL_COLOR_BLACK);
    hal_display_set_pixel(10, 10, HAL_COLOR_WHITE);
    hal_display_draw_line(0, 0, 100, 100, HAL_COLOR_RED);
    hal_display_fill_rect(50, 50, 100, 100, HAL_COLOR_BLUE);
    
    // Test text operations
    hal_display_set_text_color(HAL_COLOR_WHITE);
    hal_display_set_text_size(HAL_FONT_SIZE_MEDIUM);
    hal_display_draw_text(10, 10, "Test", HAL_COLOR_WHITE, HAL_FONT_SIZE_MEDIUM);
    
    // Clean up
    hal_display_deinit();
}

void test_hal_system_basic_operations(void) {
    // Test system initialization
    TEST_ASSERT_TRUE(hal_system_init());
    
    // Test delay function
    uint32_t start_time = hal_system_get_uptime_ms();
    hal_system_delay_ms(10);
    uint32_t end_time = hal_system_get_uptime_ms();
    
    // Should have at least 10ms difference
    TEST_ASSERT_GREATER_OR_EQUAL(10, end_time - start_time);
    
    // Test memory info
    uint32_t free_heap = hal_system_get_free_heap();
    TEST_ASSERT_GREATER_THAN(0, free_heap);
    
    // Clean up
    hal_system_deinit();
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_hal_display_init);
    RUN_TEST(test_hal_display_basic_operations);
    RUN_TEST(test_hal_system_basic_operations);
    
    return UNITY_END();
}
