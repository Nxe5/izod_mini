#ifndef PLUGIN_API_H
#define PLUGIN_API_H

/*
 * Izod Mini Plugin API
 * Flipper Zero-style plugin system for dynamic application loading
 * 
 * This API provides a hardware abstraction layer and plugin lifecycle
 * management for community-developed applications.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Plugin System Version and Compatibility
// =============================================================================
#define PLUGIN_API_VERSION_MAJOR    1
#define PLUGIN_API_VERSION_MINOR    0
#define PLUGIN_API_VERSION_PATCH    0
#define PLUGIN_API_VERSION_STRING   "1.0.0"

// Plugin compatibility flags
#define PLUGIN_COMPAT_TOUCH_WHEEL   (1 << 0)
#define PLUGIN_COMPAT_AUDIO_DAC     (1 << 1)
#define PLUGIN_COMPAT_TFT_DISPLAY   (1 << 2)
#define PLUGIN_COMPAT_SD_CARD       (1 << 3)
#define PLUGIN_COMPAT_PHYSICAL_BTNS (1 << 4)
#define PLUGIN_COMPAT_WIFI          (1 << 5)
#define PLUGIN_COMPAT_BLUETOOTH     (1 << 6)
#define PLUGIN_COMPAT_LF_RFID       (1 << 7)

// =============================================================================
// Plugin Manifest Structure
// =============================================================================
#define PLUGIN_NAME_MAX_LENGTH      32
#define PLUGIN_VERSION_MAX_LENGTH   16
#define PLUGIN_AUTHOR_MAX_LENGTH    32
#define PLUGIN_DESC_MAX_LENGTH      128
#define PLUGIN_ICON_MAX_SIZE        1024

typedef enum {
    PLUGIN_CATEGORY_MUSIC = 0,      // Music players, visualizers, equalizers
    PLUGIN_CATEGORY_PENTEST,        // WiFi scanners, BLE analyzers, RFID tools
    PLUGIN_CATEGORY_UTILITY,        // File managers, calculators, system tools
    PLUGIN_CATEGORY_GAME,           // Simple games using touch wheel and display
    PLUGIN_CATEGORY_SYSTEM,         // System applications and settings
    PLUGIN_CATEGORY_OTHER,          // Miscellaneous applications
    PLUGIN_CATEGORY_COUNT
} plugin_category_t;

typedef enum {
    PLUGIN_STATE_UNLOADED = 0,      // Plugin not loaded
    PLUGIN_STATE_LOADING,           // Plugin being loaded
    PLUGIN_STATE_LOADED,            // Plugin loaded but not running
    PLUGIN_STATE_RUNNING,           // Plugin actively running
    PLUGIN_STATE_SUSPENDED,         // Plugin suspended (backgrounded)
    PLUGIN_STATE_ERROR              // Plugin in error state
} plugin_state_t;

// Forward declarations
typedef struct plugin_context plugin_context_t;
typedef struct plugin_manifest plugin_manifest_t;

// Plugin function pointers
typedef bool (*plugin_init_fn)(plugin_context_t* ctx);
typedef void (*plugin_run_fn)(plugin_context_t* ctx);
typedef void (*plugin_cleanup_fn)(plugin_context_t* ctx);
typedef bool (*plugin_event_fn)(plugin_context_t* ctx, uint32_t event, void* data);
typedef void (*plugin_suspend_fn)(plugin_context_t* ctx);
typedef void (*plugin_resume_fn)(plugin_context_t* ctx);

// Plugin manifest structure
struct plugin_manifest {
    // Basic information
    char name[PLUGIN_NAME_MAX_LENGTH];
    char version[PLUGIN_VERSION_MAX_LENGTH];
    char author[PLUGIN_AUTHOR_MAX_LENGTH];
    char description[PLUGIN_DESC_MAX_LENGTH];
    
    // Plugin metadata
    plugin_category_t category;
    uint32_t compatibility_flags;   // Required hardware features
    uint32_t api_version;          // Required API version
    size_t memory_required;        // Minimum memory requirement (bytes)
    
    // Plugin lifecycle functions
    plugin_init_fn init;           // Initialize plugin
    plugin_run_fn run;             // Main plugin loop
    plugin_cleanup_fn cleanup;     // Cleanup resources
    plugin_event_fn event_handler; // Handle system events
    plugin_suspend_fn suspend;     // Suspend plugin (optional)
    plugin_resume_fn resume;       // Resume plugin (optional)
    
    // Plugin icon (optional)
    const uint8_t* icon_data;      // Icon bitmap data
    size_t icon_size;              // Icon data size
    uint16_t icon_width;           // Icon width in pixels
    uint16_t icon_height;          // Icon height in pixels
};

// =============================================================================
// Plugin Context and Runtime Data
// =============================================================================
struct plugin_context {
    // Plugin identification
    const plugin_manifest_t* manifest;
    plugin_state_t state;
    uint32_t plugin_id;
    
    // Runtime data
    void* private_data;            // Plugin private data
    size_t private_data_size;      // Size of private data
    uint32_t runtime_flags;        // Runtime flags
    
    // Resource tracking
    uint32_t memory_used;          // Current memory usage
    uint32_t cpu_time_ms;          // CPU time used
    uint32_t last_activity;        // Last activity timestamp
    
    // Error handling
    int last_error_code;           // Last error code
    char last_error_msg[64];       // Last error message
};

// =============================================================================
// System Events
// =============================================================================
#define PLUGIN_EVENT_INIT           0x0001
#define PLUGIN_EVENT_CLEANUP        0x0002
#define PLUGIN_EVENT_SUSPEND        0x0003
#define PLUGIN_EVENT_RESUME         0x0004
#define PLUGIN_EVENT_BUTTON_PRESS   0x0010
#define PLUGIN_EVENT_BUTTON_RELEASE 0x0011
#define PLUGIN_EVENT_TOUCH_WHEEL    0x0020
#define PLUGIN_EVENT_AUDIO_COMPLETE 0x0030
#define PLUGIN_EVENT_SD_INSERTED    0x0040
#define PLUGIN_EVENT_SD_REMOVED     0x0041
#define PLUGIN_EVENT_LOW_BATTERY    0x0050
#define PLUGIN_EVENT_SYSTEM_SLEEP   0x0060

// Button event data
typedef struct {
    uint8_t button_id;             // Button identifier
    bool long_press;               // True if long press
    uint32_t press_duration_ms;    // Press duration
} plugin_button_event_t;

// Touch wheel event data
typedef struct {
    int16_t delta;                 // Wheel movement delta
    uint16_t position;             // Absolute wheel position
    uint8_t touched_electrodes;    // Bitmask of touched electrodes
} plugin_touch_event_t;

// =============================================================================
// Hardware Abstraction Layer (HAL)
// =============================================================================

// Display HAL
typedef struct {
    void (*clear)(uint16_t color);
    void (*pixel)(int16_t x, int16_t y, uint16_t color);
    void (*line)(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void (*rect)(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void (*fill_rect)(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void (*circle)(int16_t x, int16_t y, int16_t r, uint16_t color);
    void (*fill_circle)(int16_t x, int16_t y, int16_t r, uint16_t color);
    void (*text)(int16_t x, int16_t y, const char* text, uint16_t color, uint8_t size);
    void (*bitmap)(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color);
    void (*update)(void);          // Refresh display
    uint16_t width;
    uint16_t height;
} plugin_display_hal_t;

// Audio HAL
typedef struct {
    bool (*play_tone)(uint16_t frequency, uint32_t duration_ms);
    bool (*play_wav)(const char* filename);
    bool (*play_mp3)(const char* filename);
    void (*stop)(void);
    void (*pause)(void);
    void (*resume)(void);
    void (*set_volume)(uint8_t volume);
    uint8_t (*get_volume)(void);
    bool (*is_playing)(void);
} plugin_audio_hal_t;

// Storage HAL
typedef struct {
    bool (*exists)(const char* path);
    void* (*open)(const char* path, const char* mode);
    void (*close)(void* file);
    size_t (*read)(void* file, void* buffer, size_t size);
    size_t (*write)(void* file, const void* buffer, size_t size);
    bool (*seek)(void* file, long offset, int whence);
    long (*tell)(void* file);
    bool (*mkdir)(const char* path);
    bool (*remove)(const char* path);
    bool (*rename)(const char* old_path, const char* new_path);
} plugin_storage_hal_t;

// Input HAL
typedef struct {
    bool (*is_button_pressed)(uint8_t button_id);
    int16_t (*get_wheel_delta)(void);
    uint16_t (*get_wheel_position)(void);
    uint8_t (*get_touched_electrodes)(void);
} plugin_input_hal_t;

// System HAL
typedef struct {
    uint32_t (*get_time_ms)(void);
    void (*delay_ms)(uint32_t ms);
    void (*log)(const char* level, const char* message);
    uint32_t (*get_free_heap)(void);
    uint8_t (*get_battery_level)(void);
    bool (*is_charging)(void);
    void (*request_sleep)(void);
} plugin_system_hal_t;

// Complete HAL structure
typedef struct {
    plugin_display_hal_t* display;
    plugin_audio_hal_t* audio;
    plugin_storage_hal_t* storage;
    plugin_input_hal_t* input;
    plugin_system_hal_t* system;
} plugin_hal_t;

// =============================================================================
// Plugin Manager API
// =============================================================================

/**
 * @brief Initialize the plugin system
 * @return true if successful, false otherwise
 */
bool plugin_manager_init(void);

/**
 * @brief Load a plugin from SD card
 * @param plugin_path Path to plugin directory
 * @return Plugin ID if successful, 0 if failed
 */
uint32_t plugin_manager_load(const char* plugin_path);

/**
 * @brief Unload a plugin
 * @param plugin_id Plugin ID to unload
 * @return true if successful, false otherwise
 */
bool plugin_manager_unload(uint32_t plugin_id);

/**
 * @brief Run a loaded plugin
 * @param plugin_id Plugin ID to run
 * @return true if successful, false otherwise
 */
bool plugin_manager_run(uint32_t plugin_id);

/**
 * @brief Stop a running plugin
 * @param plugin_id Plugin ID to stop
 * @return true if successful, false otherwise
 */
bool plugin_manager_stop(uint32_t plugin_id);

/**
 * @brief Suspend a running plugin
 * @param plugin_id Plugin ID to suspend
 * @return true if successful, false otherwise
 */
bool plugin_manager_suspend(uint32_t plugin_id);

/**
 * @brief Resume a suspended plugin
 * @param plugin_id Plugin ID to resume
 * @return true if successful, false otherwise
 */
bool plugin_manager_resume(uint32_t plugin_id);

/**
 * @brief Send event to a plugin
 * @param plugin_id Plugin ID
 * @param event Event type
 * @param data Event data (optional)
 * @return true if event was handled, false otherwise
 */
bool plugin_manager_send_event(uint32_t plugin_id, uint32_t event, void* data);

/**
 * @brief Get plugin context
 * @param plugin_id Plugin ID
 * @return Plugin context or NULL if not found
 */
plugin_context_t* plugin_manager_get_context(uint32_t plugin_id);

/**
 * @brief Get list of loaded plugins
 * @param plugin_ids Array to store plugin IDs
 * @param max_count Maximum number of plugin IDs to return
 * @return Number of plugins returned
 */
uint32_t plugin_manager_get_loaded_plugins(uint32_t* plugin_ids, uint32_t max_count);

/**
 * @brief Scan for available plugins on SD card
 * @param plugin_paths Array to store plugin paths
 * @param max_count Maximum number of paths to return
 * @return Number of plugins found
 */
uint32_t plugin_manager_scan_plugins(char plugin_paths[][256], uint32_t max_count);

/**
 * @brief Get plugin HAL interface
 * @return Pointer to HAL structure
 */
const plugin_hal_t* plugin_get_hal(void);

// =============================================================================
// Plugin Development Helpers
// =============================================================================

/**
 * @brief Create plugin manifest (helper for plugin developers)
 * @param name Plugin name
 * @param version Plugin version
 * @param author Plugin author
 * @param category Plugin category
 * @param init_fn Initialize function
 * @param run_fn Run function
 * @param cleanup_fn Cleanup function
 * @return Initialized manifest structure
 */
#define PLUGIN_MANIFEST(name, version, author, category, init_fn, run_fn, cleanup_fn) \
    { \
        .name = name, \
        .version = version, \
        .author = author, \
        .description = "", \
        .category = category, \
        .compatibility_flags = 0, \
        .api_version = (PLUGIN_API_VERSION_MAJOR << 16) | (PLUGIN_API_VERSION_MINOR << 8) | PLUGIN_API_VERSION_PATCH, \
        .memory_required = 0, \
        .init = init_fn, \
        .run = run_fn, \
        .cleanup = cleanup_fn, \
        .event_handler = NULL, \
        .suspend = NULL, \
        .resume = NULL, \
        .icon_data = NULL, \
        .icon_size = 0, \
        .icon_width = 0, \
        .icon_height = 0 \
    }

/**
 * @brief Plugin entry point macro
 * Use this macro to define the main entry point for your plugin
 */
#define PLUGIN_ENTRY_POINT(manifest_var) \
    extern "C" const plugin_manifest_t* plugin_get_manifest(void) { \
        return &manifest_var; \
    }

// =============================================================================
// Color Definitions for Display
// =============================================================================
#define PLUGIN_COLOR_BLACK      0x0000
#define PLUGIN_COLOR_WHITE      0xFFFF
#define PLUGIN_COLOR_RED        0xF800
#define PLUGIN_COLOR_GREEN      0x07E0
#define PLUGIN_COLOR_BLUE       0x001F
#define PLUGIN_COLOR_YELLOW     0xFFE0
#define PLUGIN_COLOR_CYAN       0x07FF
#define PLUGIN_COLOR_MAGENTA    0xF81F
#define PLUGIN_COLOR_GRAY       0x8410
#define PLUGIN_COLOR_DARK_GRAY  0x4208
#define PLUGIN_COLOR_LIGHT_GRAY 0xC618

// =============================================================================
// Button Definitions
// =============================================================================
#define PLUGIN_BUTTON_PREVIOUS  0
#define PLUGIN_BUTTON_PLAY      1
#define PLUGIN_BUTTON_NEXT      2
#define PLUGIN_BUTTON_MENU      3
#define PLUGIN_BUTTON_SELECT    4

#ifdef __cplusplus
}
#endif

#endif // PLUGIN_API_H

