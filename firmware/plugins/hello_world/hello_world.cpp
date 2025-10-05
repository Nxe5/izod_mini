/*
 * Hello World Plugin
 * Simple example plugin demonstrating the Izod Mini plugin API
 */

#include "plugin_api.h"
#include <Arduino.h>

// Plugin state structure
typedef struct {
    uint32_t counter;
    uint32_t last_update;
    bool button_pressed;
    int16_t wheel_position;
} hello_world_state_t;

// Plugin functions
static bool hello_world_init(plugin_context_t* ctx);
static void hello_world_run(plugin_context_t* ctx);
static void hello_world_cleanup(plugin_context_t* ctx);
static bool hello_world_event_handler(plugin_context_t* ctx, uint32_t event, void* data);

// Plugin manifest
static const plugin_manifest_t hello_world_manifest = PLUGIN_MANIFEST(
    "Hello World",
    "1.0.0", 
    "Izod Mini Team",
    PLUGIN_CATEGORY_OTHER,
    hello_world_init,
    hello_world_run,
    hello_world_cleanup
);

// Set event handler
static plugin_manifest_t hello_world_manifest_with_events = hello_world_manifest;

// Plugin initialization
static bool hello_world_init(plugin_context_t* ctx) {
    if (!ctx) return false;
    
    // Allocate plugin state
    hello_world_state_t* state = (hello_world_state_t*)malloc(sizeof(hello_world_state_t));
    if (!state) {
        return false;
    }
    
    // Initialize state
    state->counter = 0;
    state->last_update = 0;
    state->button_pressed = false;
    state->wheel_position = 0;
    
    // Store state in context
    ctx->private_data = state;
    ctx->private_data_size = sizeof(hello_world_state_t);
    
    // Get HAL interface
    const plugin_hal_t* hal = plugin_get_hal();
    if (!hal || !hal->system) {
        free(state);
        return false;
    }
    
    // Log initialization
    hal->system->log("INFO", "Hello World plugin initialized");
    
    return true;
}

// Plugin main loop
static void hello_world_run(plugin_context_t* ctx) {
    if (!ctx || !ctx->private_data) return;
    
    hello_world_state_t* state = (hello_world_state_t*)ctx->private_data;
    const plugin_hal_t* hal = plugin_get_hal();
    
    if (!hal) return;
    
    uint32_t now = hal->system->get_time_ms();
    
    // Update display every 100ms
    if (now - state->last_update > 100) {
        // Clear display
        hal->display->clear(PLUGIN_COLOR_BLACK);
        
        // Draw title
        hal->display->text(10, 10, "Hello World Plugin", PLUGIN_COLOR_WHITE, 2);
        
        // Draw counter
        char counter_text[32];
        snprintf(counter_text, sizeof(counter_text), "Counter: %lu", state->counter);
        hal->display->text(10, 40, counter_text, PLUGIN_COLOR_GREEN, 1);
        
        // Draw system info
        char heap_text[32];
        snprintf(heap_text, sizeof(heap_text), "Free Heap: %lu", hal->system->get_free_heap());
        hal->display->text(10, 60, heap_text, PLUGIN_COLOR_CYAN, 1);
        
        char battery_text[32];
        snprintf(battery_text, sizeof(battery_text), "Battery: %d%%", hal->system->get_battery_level());
        hal->display->text(10, 80, battery_text, PLUGIN_COLOR_YELLOW, 1);
        
        // Draw button status
        hal->display->text(10, 100, "Buttons:", PLUGIN_COLOR_WHITE, 1);
        for (int i = 0; i < 5; i++) {
            bool pressed = hal->input->is_button_pressed(i);
            hal->display->fill_rect(10 + i * 25, 120, 20, 15, 
                                  pressed ? PLUGIN_COLOR_RED : PLUGIN_COLOR_DARK_GRAY);
            
            char btn_text[4];
            snprintf(btn_text, sizeof(btn_text), "%d", i);
            hal->display->text(15 + i * 25, 125, btn_text, PLUGIN_COLOR_WHITE, 1);
        }
        
        // Draw touch wheel info
        char wheel_text[32];
        snprintf(wheel_text, sizeof(wheel_text), "Wheel: %d", state->wheel_position);
        hal->display->text(10, 150, wheel_text, PLUGIN_COLOR_MAGENTA, 1);
        
        // Draw touch electrodes
        uint8_t touched = hal->input->get_touched_electrodes();
        hal->display->text(10, 170, "Touch:", PLUGIN_COLOR_WHITE, 1);
        for (int i = 0; i < 12; i++) {
            bool is_touched = (touched & (1 << i)) != 0;
            hal->display->fill_circle(70 + (i % 6) * 25, 180 + (i / 6) * 25, 8,
                                    is_touched ? PLUGIN_COLOR_GREEN : PLUGIN_COLOR_GRAY);
        }
        
        // Draw instructions
        hal->display->text(10, 230, "Press buttons or use wheel", PLUGIN_COLOR_LIGHT_GRAY, 1);
        hal->display->text(10, 250, "Menu button to exit", PLUGIN_COLOR_LIGHT_GRAY, 1);
        
        // Update display
        hal->display->update();
        
        state->last_update = now;
        state->counter++;
    }
    
    // Check for wheel movement
    int16_t wheel_delta = hal->input->get_wheel_delta();
    if (wheel_delta != 0) {
        state->wheel_position += wheel_delta;
        // Keep position in reasonable range
        if (state->wheel_position < 0) state->wheel_position = 0;
        if (state->wheel_position > 360) state->wheel_position = 360;
    }
    
    // Small delay to prevent excessive CPU usage
    hal->system->delay_ms(10);
}

// Plugin cleanup
static void hello_world_cleanup(plugin_context_t* ctx) {
    if (!ctx) return;
    
    const plugin_hal_t* hal = plugin_get_hal();
    if (hal && hal->system) {
        hal->system->log("INFO", "Hello World plugin cleanup");
    }
    
    // Free plugin state
    if (ctx->private_data) {
        free(ctx->private_data);
        ctx->private_data = nullptr;
        ctx->private_data_size = 0;
    }
}

// Plugin event handler
static bool hello_world_event_handler(plugin_context_t* ctx, uint32_t event, void* data) {
    if (!ctx || !ctx->private_data) return false;
    
    hello_world_state_t* state = (hello_world_state_t*)ctx->private_data;
    const plugin_hal_t* hal = plugin_get_hal();
    
    switch (event) {
        case PLUGIN_EVENT_BUTTON_PRESS: {
            plugin_button_event_t* btn_event = (plugin_button_event_t*)data;
            if (btn_event) {
                state->button_pressed = true;
                
                // Handle specific buttons
                if (btn_event->button_id == PLUGIN_BUTTON_MENU) {
                    // Exit plugin on menu button
                    if (hal && hal->system) {
                        hal->system->log("INFO", "Menu button pressed - exiting plugin");
                    }
                    return false; // Signal plugin should exit
                }
                
                // Play tone on button press
                if (hal && hal->audio) {
                    uint16_t frequency = 440 + (btn_event->button_id * 110); // Different tone per button
                    hal->audio->play_tone(frequency, 100);
                }
            }
            return true;
        }
        
        case PLUGIN_EVENT_BUTTON_RELEASE: {
            state->button_pressed = false;
            return true;
        }
        
        case PLUGIN_EVENT_TOUCH_WHEEL: {
            plugin_touch_event_t* touch_event = (plugin_touch_event_t*)data;
            if (touch_event) {
                state->wheel_position = touch_event->position;
                
                // Play tone based on wheel position
                if (hal && hal->audio && touch_event->delta != 0) {
                    uint16_t frequency = 200 + (touch_event->position * 2);
                    hal->audio->play_tone(frequency, 50);
                }
            }
            return true;
        }
        
        case PLUGIN_EVENT_LOW_BATTERY: {
            if (hal && hal->display) {
                // Flash red warning
                hal->display->fill_rect(0, 0, hal->display->width, 20, PLUGIN_COLOR_RED);
                hal->display->text(10, 5, "LOW BATTERY!", PLUGIN_COLOR_WHITE, 1);
                hal->display->update();
            }
            return true;
        }
        
        default:
            return false; // Event not handled
    }
}

// Plugin entry point
extern "C" {
    const plugin_manifest_t* plugin_get_manifest(void) {
        // Set event handler
        hello_world_manifest_with_events.event_handler = hello_world_event_handler;
        hello_world_manifest_with_events.compatibility_flags = PLUGIN_COMPAT_TFT_DISPLAY | PLUGIN_COMPAT_PHYSICAL_BTNS;
        hello_world_manifest_with_events.memory_required = 8192;
        
        // Set description
        strncpy((char*)hello_world_manifest_with_events.description, 
                "Demonstrates plugin API features including display, input, and audio", 
                PLUGIN_DESC_MAX_LENGTH - 1);
        
        return &hello_world_manifest_with_events;
    }
}

