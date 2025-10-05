/*
 * System Monitor Plugin
 * Real-time system monitoring and diagnostics
 */

#include "plugin_api.h"
#include <Arduino.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <soc/rtc.h>

// Plugin state
typedef struct {
    int selected_item;
    uint32_t last_update;
    uint32_t update_interval_ms;
    
    // System metrics
    struct {
        uint32_t free_heap;
        uint32_t min_free_heap;
        uint32_t total_heap;
        uint32_t free_psram;
        uint32_t total_psram;
        uint8_t cpu_freq_mhz;
        float temperature;
        uint32_t uptime_seconds;
        uint8_t battery_level;
        bool charging;
        uint32_t flash_size;
        uint32_t flash_speed;
        uint8_t reset_reason;
        uint32_t task_count;
    } metrics;
    
    // Performance history (last 60 samples)
    struct {
        uint32_t heap_history[60];
        uint8_t cpu_history[60];
        uint8_t battery_history[60];
        int history_index;
        bool history_full;
    } history;
    
    // UI state
    enum {
        STATE_OVERVIEW,
        STATE_MEMORY,
        STATE_PERFORMANCE,
        STATE_HARDWARE,
        STATE_TASKS,
        STATE_SETTINGS
    } ui_state;
    
    // Settings
    bool auto_refresh;
    uint16_t refresh_rate_ms;
    bool show_graphs;
    
} system_monitor_state_t;

// Function prototypes
static bool system_monitor_init(plugin_context_t* ctx);
static void system_monitor_run(plugin_context_t* ctx);
static void system_monitor_cleanup(plugin_context_t* ctx);
static bool system_monitor_event_handler(plugin_context_t* ctx, uint32_t event, void* data);

// Internal functions
static void update_system_metrics(system_monitor_state_t* state);
static void update_performance_history(system_monitor_state_t* state);
static void draw_overview(system_monitor_state_t* state, const plugin_hal_t* hal);
static void draw_memory_info(system_monitor_state_t* state, const plugin_hal_t* hal);
static void draw_performance_graphs(system_monitor_state_t* state, const plugin_hal_t* hal);
static void draw_hardware_info(system_monitor_state_t* state, const plugin_hal_t* hal);
static void draw_task_info(system_monitor_state_t* state, const plugin_hal_t* hal);
static void draw_settings(system_monitor_state_t* state, const plugin_hal_t* hal);
static void draw_progress_bar(const plugin_hal_t* hal, int x, int y, int width, int height, 
                             int value, int max_value, uint16_t color);
static void draw_mini_graph(const plugin_hal_t* hal, int x, int y, int width, int height,
                           uint32_t* data, int data_count, uint32_t max_value, uint16_t color);
static const char* get_reset_reason_string(uint8_t reason);
static float get_cpu_temperature();

// Plugin manifest
static plugin_manifest_t system_monitor_manifest = PLUGIN_MANIFEST(
    "System Monitor",
    "1.0.0",
    "Izod Mini Team",
    PLUGIN_CATEGORY_UTILITY,
    system_monitor_init,
    system_monitor_run,
    system_monitor_cleanup
);

// Plugin initialization
static bool system_monitor_init(plugin_context_t* ctx) {
    if (!ctx) return false;
    
    // Allocate plugin state
    system_monitor_state_t* state = (system_monitor_state_t*)malloc(sizeof(system_monitor_state_t));
    if (!state) return false;
    
    // Initialize state
    memset(state, 0, sizeof(system_monitor_state_t));
    state->ui_state = STATE_OVERVIEW;
    state->selected_item = 0;
    state->update_interval_ms = 1000; // 1 second
    state->auto_refresh = true;
    state->refresh_rate_ms = 1000;
    state->show_graphs = true;
    
    ctx->private_data = state;
    ctx->private_data_size = sizeof(system_monitor_state_t);
    
    // Initial metrics update
    update_system_metrics(state);
    
    const plugin_hal_t* hal = plugin_get_hal();
    if (hal && hal->system) {
        hal->system->log("INFO", "System Monitor initialized");
    }
    
    return true;
}

// Plugin main loop
static void system_monitor_run(plugin_context_t* ctx) {
    if (!ctx || !ctx->private_data) return;
    
    system_monitor_state_t* state = (system_monitor_state_t*)ctx->private_data;
    const plugin_hal_t* hal = plugin_get_hal();
    if (!hal) return;
    
    uint32_t now = hal->system->get_time_ms();
    
    // Update metrics periodically
    if (state->auto_refresh && (now - state->last_update > state->refresh_rate_ms)) {
        update_system_metrics(state);
        update_performance_history(state);
        state->last_update = now;
    }
    
    // Update display every 100ms
    static uint32_t last_display_update = 0;
    if (now - last_display_update > 100) {
        hal->display->clear(PLUGIN_COLOR_BLACK);
        
        switch (state->ui_state) {
            case STATE_OVERVIEW:
                draw_overview(state, hal);
                break;
            case STATE_MEMORY:
                draw_memory_info(state, hal);
                break;
            case STATE_PERFORMANCE:
                draw_performance_graphs(state, hal);
                break;
            case STATE_HARDWARE:
                draw_hardware_info(state, hal);
                break;
            case STATE_TASKS:
                draw_task_info(state, hal);
                break;
            case STATE_SETTINGS:
                draw_settings(state, hal);
                break;
        }
        
        hal->display->update();
        last_display_update = now;
    }
    
    hal->system->delay_ms(10);
}

// Plugin cleanup
static void system_monitor_cleanup(plugin_context_t* ctx) {
    if (!ctx || !ctx->private_data) return;
    
    const plugin_hal_t* hal = plugin_get_hal();
    if (hal && hal->system) {
        hal->system->log("INFO", "System Monitor cleanup");
    }
    
    free(ctx->private_data);
    ctx->private_data = nullptr;
    ctx->private_data_size = 0;
}

// Plugin event handler
static bool system_monitor_event_handler(plugin_context_t* ctx, uint32_t event, void* data) {
    if (!ctx || !ctx->private_data) return false;
    
    system_monitor_state_t* state = (system_monitor_state_t*)ctx->private_data;
    
    if (event == PLUGIN_EVENT_BUTTON_PRESS) {
        plugin_button_event_t* btn_event = (plugin_button_event_t*)data;
        if (!btn_event) return false;
        
        switch (btn_event->button_id) {
            case PLUGIN_BUTTON_MENU:
                return false; // Exit plugin
                
            case PLUGIN_BUTTON_SELECT:
                // Cycle through views
                state->ui_state = (decltype(state->ui_state))((state->ui_state + 1) % 6);
                state->selected_item = 0;
                break;
                
            case PLUGIN_BUTTON_PREVIOUS:
                switch (state->ui_state) {
                    case STATE_SETTINGS:
                        state->selected_item = (state->selected_item - 1 + 3) % 3;
                        break;
                    default:
                        // Manual refresh
                        update_system_metrics(state);
                        update_performance_history(state);
                        break;
                }
                break;
                
            case PLUGIN_BUTTON_NEXT:
                switch (state->ui_state) {
                    case STATE_SETTINGS:
                        state->selected_item = (state->selected_item + 1) % 3;
                        break;
                    default:
                        // Manual refresh
                        update_system_metrics(state);
                        update_performance_history(state);
                        break;
                }
                break;
                
            case PLUGIN_BUTTON_PLAY:
                switch (state->ui_state) {
                    case STATE_SETTINGS:
                        // Toggle settings
                        switch (state->selected_item) {
                            case 0: // Auto refresh
                                state->auto_refresh = !state->auto_refresh;
                                break;
                            case 1: // Refresh rate
                                state->refresh_rate_ms = (state->refresh_rate_ms == 1000) ? 500 :
                                                        (state->refresh_rate_ms == 500) ? 2000 : 1000;
                                break;
                            case 2: // Show graphs
                                state->show_graphs = !state->show_graphs;
                                break;
                        }
                        break;
                    default:
                        // Toggle auto refresh
                        state->auto_refresh = !state->auto_refresh;
                        break;
                }
                break;
        }
        
        return true;
    }
    
    return false;
}

// Internal functions
static void update_system_metrics(system_monitor_state_t* state) {
    // Memory info
    state->metrics.free_heap = ESP.getFreeHeap();
    state->metrics.min_free_heap = ESP.getMinFreeHeap();
    state->metrics.total_heap = ESP.getHeapSize();
    state->metrics.free_psram = ESP.getFreePsram();
    state->metrics.total_psram = ESP.getPsramSize();
    
    // CPU info
    state->metrics.cpu_freq_mhz = ESP.getCpuFreqMHz();
    state->metrics.temperature = get_cpu_temperature();
    
    // System info
    state->metrics.uptime_seconds = millis() / 1000;
    state->metrics.flash_size = ESP.getFlashChipSize();
    state->metrics.flash_speed = ESP.getFlashChipSpeed();
    state->metrics.reset_reason = esp_reset_reason();
    
    // Task count (approximate)
    state->metrics.task_count = uxTaskGetNumberOfTasks();
    
    // Battery info (would need actual hardware implementation)
    state->metrics.battery_level = 85; // Placeholder
    state->metrics.charging = false;   // Placeholder
}

static void update_performance_history(system_monitor_state_t* state) {
    // Update heap history
    state->history.heap_history[state->history.history_index] = state->metrics.free_heap;
    
    // Update CPU usage history (simplified calculation)
    uint8_t cpu_usage = 100 - ((state->metrics.free_heap * 100) / state->metrics.total_heap);
    state->history.cpu_history[state->history.history_index] = cpu_usage;
    
    // Update battery history
    state->history.battery_history[state->history.history_index] = state->metrics.battery_level;
    
    // Advance index
    state->history.history_index = (state->history.history_index + 1) % 60;
    if (state->history.history_index == 0) {
        state->history.history_full = true;
    }
}

static void draw_overview(system_monitor_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "System Overview", PLUGIN_COLOR_WHITE, 2);
    
    // Uptime
    uint32_t uptime = state->metrics.uptime_seconds;
    uint32_t hours = uptime / 3600;
    uint32_t minutes = (uptime % 3600) / 60;
    uint32_t seconds = uptime % 60;
    char uptime_str[30];
    snprintf(uptime_str, sizeof(uptime_str), "Uptime: %02lu:%02lu:%02lu", hours, minutes, seconds);
    hal->display->text(10, 40, uptime_str, PLUGIN_COLOR_GREEN, 1);
    
    // Memory usage
    uint32_t used_heap = state->metrics.total_heap - state->metrics.free_heap;
    uint8_t heap_percent = (used_heap * 100) / state->metrics.total_heap;
    char heap_str[40];
    snprintf(heap_str, sizeof(heap_str), "Heap: %lu/%lu KB (%d%%)", 
             used_heap / 1024, state->metrics.total_heap / 1024, heap_percent);
    hal->display->text(10, 60, heap_str, PLUGIN_COLOR_CYAN, 1);
    
    // Memory bar
    draw_progress_bar(hal, 10, 75, 200, 8, used_heap, state->metrics.total_heap, PLUGIN_COLOR_CYAN);
    
    // CPU frequency
    char cpu_str[30];
    snprintf(cpu_str, sizeof(cpu_str), "CPU: %d MHz", state->metrics.cpu_freq_mhz);
    hal->display->text(10, 95, cpu_str, PLUGIN_COLOR_YELLOW, 1);
    
    // Temperature
    if (state->metrics.temperature > 0) {
        char temp_str[20];
        snprintf(temp_str, sizeof(temp_str), "Temp: %.1f°C", state->metrics.temperature);
        hal->display->text(120, 95, temp_str, PLUGIN_COLOR_YELLOW, 1);
    }
    
    // Battery
    char battery_str[30];
    snprintf(battery_str, sizeof(battery_str), "Battery: %d%% %s", 
             state->metrics.battery_level, state->metrics.charging ? "(Charging)" : "");
    hal->display->text(10, 115, battery_str, PLUGIN_COLOR_GREEN, 1);
    
    // Battery bar
    draw_progress_bar(hal, 10, 130, 200, 8, state->metrics.battery_level, 100, PLUGIN_COLOR_GREEN);
    
    // Tasks
    char task_str[20];
    snprintf(task_str, sizeof(task_str), "Tasks: %lu", state->metrics.task_count);
    hal->display->text(10, 150, task_str, PLUGIN_COLOR_WHITE, 1);
    
    // Flash info
    char flash_str[40];
    snprintf(flash_str, sizeof(flash_str), "Flash: %lu MB @ %lu MHz", 
             state->metrics.flash_size / (1024 * 1024), 
             state->metrics.flash_speed / 1000000);
    hal->display->text(10, 170, flash_str, PLUGIN_COLOR_WHITE, 1);
    
    // Mini performance graphs
    if (state->show_graphs && state->history.history_full) {
        hal->display->text(10, 200, "Heap History:", PLUGIN_COLOR_GRAY, 1);
        draw_mini_graph(hal, 10, 215, 100, 30, state->history.heap_history, 60, 
                       state->metrics.total_heap, PLUGIN_COLOR_CYAN);
        
        hal->display->text(120, 200, "CPU Usage:", PLUGIN_COLOR_GRAY, 1);
        draw_mini_graph(hal, 120, 215, 100, 30, (uint32_t*)state->history.cpu_history, 60, 
                       100, PLUGIN_COLOR_YELLOW);
    }
    
    // Instructions
    hal->display->text(10, 280, "Select: Next view  Play: Toggle auto", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 295, "Prev/Next: Refresh  Menu: Exit", PLUGIN_COLOR_GRAY, 1);
}

static void draw_memory_info(system_monitor_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "Memory Details", PLUGIN_COLOR_WHITE, 2);
    
    // Heap memory
    hal->display->text(10, 40, "Heap Memory:", PLUGIN_COLOR_WHITE, 1);
    char heap_total[30];
    snprintf(heap_total, sizeof(heap_total), "Total: %lu KB", state->metrics.total_heap / 1024);
    hal->display->text(20, 60, heap_total, PLUGIN_COLOR_CYAN, 1);
    
    char heap_free[30];
    snprintf(heap_free, sizeof(heap_free), "Free: %lu KB", state->metrics.free_heap / 1024);
    hal->display->text(20, 80, heap_free, PLUGIN_COLOR_GREEN, 1);
    
    char heap_used[30];
    uint32_t used_heap = state->metrics.total_heap - state->metrics.free_heap;
    snprintf(heap_used, sizeof(heap_used), "Used: %lu KB", used_heap / 1024);
    hal->display->text(20, 100, heap_used, PLUGIN_COLOR_RED, 1);
    
    char heap_min[30];
    snprintf(heap_min, sizeof(heap_min), "Min Free: %lu KB", state->metrics.min_free_heap / 1024);
    hal->display->text(20, 120, heap_min, PLUGIN_COLOR_YELLOW, 1);
    
    // Heap usage bar
    draw_progress_bar(hal, 20, 135, 200, 12, used_heap, state->metrics.total_heap, PLUGIN_COLOR_CYAN);
    
    // PSRAM (if available)
    if (state->metrics.total_psram > 0) {
        hal->display->text(10, 160, "PSRAM:", PLUGIN_COLOR_WHITE, 1);
        char psram_total[30];
        snprintf(psram_total, sizeof(psram_total), "Total: %lu KB", state->metrics.total_psram / 1024);
        hal->display->text(20, 180, psram_total, PLUGIN_COLOR_MAGENTA, 1);
        
        char psram_free[30];
        snprintf(psram_free, sizeof(psram_free), "Free: %lu KB", state->metrics.free_psram / 1024);
        hal->display->text(20, 200, psram_free, PLUGIN_COLOR_GREEN, 1);
        
        uint32_t used_psram = state->metrics.total_psram - state->metrics.free_psram;
        draw_progress_bar(hal, 20, 215, 200, 12, used_psram, state->metrics.total_psram, PLUGIN_COLOR_MAGENTA);
    } else {
        hal->display->text(10, 160, "PSRAM: Not available", PLUGIN_COLOR_GRAY, 1);
    }
    
    // Memory fragmentation info
    uint8_t fragmentation = 100 - ((state->metrics.min_free_heap * 100) / state->metrics.free_heap);
    char frag_str[30];
    snprintf(frag_str, sizeof(frag_str), "Fragmentation: %d%%", fragmentation);
    hal->display->text(10, 240, frag_str, fragmentation > 50 ? PLUGIN_COLOR_RED : PLUGIN_COLOR_GREEN, 1);
    
    hal->display->text(10, 280, "Select: Next view", PLUGIN_COLOR_GRAY, 1);
}

static void draw_performance_graphs(system_monitor_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "Performance Graphs", PLUGIN_COLOR_WHITE, 2);
    
    if (!state->history.history_full) {
        hal->display->text(10, 50, "Collecting data...", PLUGIN_COLOR_YELLOW, 1);
        char samples_str[30];
        snprintf(samples_str, sizeof(samples_str), "Samples: %d/60", state->history.history_index);
        hal->display->text(10, 70, samples_str, PLUGIN_COLOR_GRAY, 1);
        return;
    }
    
    // Heap usage graph
    hal->display->text(10, 40, "Heap Usage (KB):", PLUGIN_COLOR_CYAN, 1);
    draw_mini_graph(hal, 10, 55, 220, 60, state->history.heap_history, 60, 
                   state->metrics.total_heap, PLUGIN_COLOR_CYAN);
    
    // CPU usage graph
    hal->display->text(10, 130, "CPU Usage (%):", PLUGIN_COLOR_YELLOW, 1);
    draw_mini_graph(hal, 10, 145, 220, 60, (uint32_t*)state->history.cpu_history, 60, 
                   100, PLUGIN_COLOR_YELLOW);
    
    // Battery level graph
    hal->display->text(10, 220, "Battery Level (%):", PLUGIN_COLOR_GREEN, 1);
    draw_mini_graph(hal, 10, 235, 220, 40, (uint32_t*)state->history.battery_history, 60, 
                   100, PLUGIN_COLOR_GREEN);
    
    hal->display->text(10, 285, "60 samples, 1 sample/sec", PLUGIN_COLOR_GRAY, 1);
}

static void draw_hardware_info(system_monitor_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "Hardware Info", PLUGIN_COLOR_WHITE, 2);
    
    // Chip info
    hal->display->text(10, 40, "Chip:", PLUGIN_COLOR_WHITE, 1);
    hal->display->text(60, 40, ESP.getChipModel(), PLUGIN_COLOR_CYAN, 1);
    
    char revision[20];
    snprintf(revision, sizeof(revision), "Rev %d", ESP.getChipRevision());
    hal->display->text(150, 40, revision, PLUGIN_COLOR_CYAN, 1);
    
    // CPU frequency
    char cpu_info[30];
    snprintf(cpu_info, sizeof(cpu_info), "CPU: %d MHz", state->metrics.cpu_freq_mhz);
    hal->display->text(10, 60, cpu_info, PLUGIN_COLOR_YELLOW, 1);
    
    // Flash info
    char flash_info[40];
    snprintf(flash_info, sizeof(flash_info), "Flash: %lu MB", state->metrics.flash_size / (1024 * 1024));
    hal->display->text(10, 80, flash_info, PLUGIN_COLOR_WHITE, 1);
    
    char flash_speed[30];
    snprintf(flash_speed, sizeof(flash_speed), "Speed: %lu MHz", state->metrics.flash_speed / 1000000);
    hal->display->text(10, 100, flash_speed, PLUGIN_COLOR_WHITE, 1);
    
    // MAC addresses
    hal->display->text(10, 130, "MAC Addresses:", PLUGIN_COLOR_WHITE, 1);
    
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char wifi_mac[20];
    snprintf(wifi_mac, sizeof(wifi_mac), "WiFi: %02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    hal->display->text(10, 150, wifi_mac, PLUGIN_COLOR_GRAY, 1);
    
    esp_read_mac(mac, ESP_MAC_BT);
    char bt_mac[20];
    snprintf(bt_mac, sizeof(bt_mac), "BT: %02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    hal->display->text(10, 170, bt_mac, PLUGIN_COLOR_GRAY, 1);
    
    // Reset reason
    hal->display->text(10, 200, "Last Reset:", PLUGIN_COLOR_WHITE, 1);
    hal->display->text(100, 200, get_reset_reason_string(state->metrics.reset_reason), PLUGIN_COLOR_YELLOW, 1);
    
    // Temperature
    if (state->metrics.temperature > 0) {
        char temp_str[20];
        snprintf(temp_str, sizeof(temp_str), "Temperature: %.1f°C", state->metrics.temperature);
        hal->display->text(10, 220, temp_str, PLUGIN_COLOR_RED, 1);
    }
    
    hal->display->text(10, 280, "Select: Next view", PLUGIN_COLOR_GRAY, 1);
}

static void draw_task_info(system_monitor_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "Task Information", PLUGIN_COLOR_WHITE, 2);
    
    char task_count[20];
    snprintf(task_count, sizeof(task_count), "Total Tasks: %lu", state->metrics.task_count);
    hal->display->text(10, 40, task_count, PLUGIN_COLOR_WHITE, 1);
    
    // Task list would require more complex FreeRTOS integration
    hal->display->text(10, 70, "Task details require", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 90, "extended FreeRTOS API", PLUGIN_COLOR_GRAY, 1);
    
    // Stack info
    hal->display->text(10, 120, "Stack Info:", PLUGIN_COLOR_WHITE, 1);
    char stack_info[30];
    snprintf(stack_info, sizeof(stack_info), "Free stack: %d bytes", uxTaskGetStackHighWaterMark(NULL));
    hal->display->text(10, 140, stack_info, PLUGIN_COLOR_GREEN, 1);
    
    // Watchdog info
    hal->display->text(10, 170, "Watchdog: Active", PLUGIN_COLOR_GREEN, 1);
    
    hal->display->text(10, 280, "Select: Next view", PLUGIN_COLOR_GRAY, 1);
}

static void draw_settings(system_monitor_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "Monitor Settings", PLUGIN_COLOR_WHITE, 2);
    
    const char* settings[] = {
        "Auto Refresh",
        "Refresh Rate",
        "Show Graphs"
    };
    
    for (int i = 0; i < 3; i++) {
        uint16_t color = (i == state->selected_item) ? PLUGIN_COLOR_GREEN : PLUGIN_COLOR_WHITE;
        hal->display->text(20, 50 + i * 25, settings[i], color, 1);
        
        if (i == state->selected_item) {
            hal->display->text(10, 50 + i * 25, ">", PLUGIN_COLOR_GREEN, 1);
        }
        
        // Setting values
        char value[20];
        switch (i) {
            case 0:
                strcpy(value, state->auto_refresh ? "ON" : "OFF");
                break;
            case 1:
                snprintf(value, sizeof(value), "%d ms", state->refresh_rate_ms);
                break;
            case 2:
                strcpy(value, state->show_graphs ? "ON" : "OFF");
                break;
        }
        hal->display->text(150, 50 + i * 25, value, color, 1);
    }
    
    hal->display->text(10, 200, "Play: Toggle setting", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 220, "Select: Next view", PLUGIN_COLOR_GRAY, 1);
}

// Helper functions
static void draw_progress_bar(const plugin_hal_t* hal, int x, int y, int width, int height, 
                             int value, int max_value, uint16_t color) {
    // Draw border
    hal->display->rect(x, y, width, height, PLUGIN_COLOR_WHITE);
    
    // Draw fill
    if (max_value > 0) {
        int fill_width = (value * (width - 2)) / max_value;
        fill_width = constrain(fill_width, 0, width - 2);
        hal->display->fill_rect(x + 1, y + 1, fill_width, height - 2, color);
    }
}

static void draw_mini_graph(const plugin_hal_t* hal, int x, int y, int width, int height,
                           uint32_t* data, int data_count, uint32_t max_value, uint16_t color) {
    // Draw border
    hal->display->rect(x, y, width, height, PLUGIN_COLOR_WHITE);
    
    if (max_value == 0) return;
    
    // Draw data points
    for (int i = 1; i < data_count; i++) {
        int x1 = x + 1 + ((i - 1) * (width - 2)) / (data_count - 1);
        int y1 = y + height - 1 - ((data[i - 1] * (height - 2)) / max_value);
        int x2 = x + 1 + (i * (width - 2)) / (data_count - 1);
        int y2 = y + height - 1 - ((data[i] * (height - 2)) / max_value);
        
        hal->display->line(x1, y1, x2, y2, color);
    }
}

static const char* get_reset_reason_string(uint8_t reason) {
    switch (reason) {
        case ESP_RST_POWERON: return "Power On";
        case ESP_RST_EXT: return "External";
        case ESP_RST_SW: return "Software";
        case ESP_RST_PANIC: return "Panic";
        case ESP_RST_INT_WDT: return "Interrupt WDT";
        case ESP_RST_TASK_WDT: return "Task WDT";
        case ESP_RST_WDT: return "Other WDT";
        case ESP_RST_DEEPSLEEP: return "Deep Sleep";
        case ESP_RST_BROWNOUT: return "Brownout";
        case ESP_RST_SDIO: return "SDIO";
        default: return "Unknown";
    }
}

static float get_cpu_temperature() {
    // ESP32 doesn't have a built-in temperature sensor accessible via Arduino
    // This would require direct register access or external sensor
    return 0.0; // Placeholder
}

// Plugin entry point
extern "C" {
    const plugin_manifest_t* plugin_get_manifest(void) {
        system_monitor_manifest.event_handler = system_monitor_event_handler;
        system_monitor_manifest.compatibility_flags = PLUGIN_COMPAT_TFT_DISPLAY | PLUGIN_COMPAT_PHYSICAL_BTNS;
        system_monitor_manifest.memory_required = 8192;
        
        strncpy((char*)system_monitor_manifest.description,
                "Real-time system monitoring with performance graphs and diagnostics",
                PLUGIN_DESC_MAX_LENGTH - 1);
        
        return &system_monitor_manifest;
    }
}

