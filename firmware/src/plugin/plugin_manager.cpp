/*
 * Plugin Manager Implementation
 * Flipper Zero-style dynamic plugin loading and management
 */

#include "plugin_api.h"
#include "hardware_config.h"
#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <vector>
#include <map>

// Maximum number of loaded plugins
#define MAX_LOADED_PLUGINS      8
#define PLUGIN_MANIFEST_FILE    "manifest.json"
#define PLUGIN_BINARY_FILE      "plugin.bin"
#define PLUGIN_ICON_FILE        "icon.bmp"

// Plugin runtime data
typedef struct {
    plugin_context_t context;
    bool active;
    uint32_t load_time;
    char plugin_path[256];
} plugin_runtime_t;

// Global plugin manager state
static bool g_plugin_manager_initialized = false;
static plugin_runtime_t g_loaded_plugins[MAX_LOADED_PLUGINS];
static uint32_t g_next_plugin_id = 1;
static uint32_t g_current_running_plugin = 0;

// Hardware Abstraction Layer instances
static plugin_display_hal_t g_display_hal;
static plugin_audio_hal_t g_audio_hal;
static plugin_storage_hal_t g_storage_hal;
static plugin_input_hal_t g_input_hal;
static plugin_system_hal_t g_system_hal;
static plugin_hal_t g_hal;

// Forward declarations
static bool load_plugin_manifest(const char* plugin_path, plugin_manifest_t* manifest);
static bool validate_plugin_compatibility(const plugin_manifest_t* manifest);
static plugin_runtime_t* find_plugin_by_id(uint32_t plugin_id);
static void init_hal_interfaces();

// =============================================================================
// HAL Implementation Functions
// =============================================================================

// Display HAL implementations
static void hal_display_clear(uint16_t color) {
    // Implementation would interface with actual display driver
    Serial.printf("Display: Clear with color 0x%04X\n", color);
}

static void hal_display_pixel(int16_t x, int16_t y, uint16_t color) {
    Serial.printf("Display: Pixel at (%d,%d) color 0x%04X\n", x, y, color);
}

static void hal_display_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    Serial.printf("Display: Line from (%d,%d) to (%d,%d) color 0x%04X\n", x0, y0, x1, y1, color);
}

static void hal_display_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    Serial.printf("Display: Rect at (%d,%d) size %dx%d color 0x%04X\n", x, y, w, h, color);
}

static void hal_display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    Serial.printf("Display: Fill rect at (%d,%d) size %dx%d color 0x%04X\n", x, y, w, h, color);
}

static void hal_display_circle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    Serial.printf("Display: Circle at (%d,%d) radius %d color 0x%04X\n", x, y, r, color);
}

static void hal_display_fill_circle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    Serial.printf("Display: Fill circle at (%d,%d) radius %d color 0x%04X\n", x, y, r, color);
}

static void hal_display_text(int16_t x, int16_t y, const char* text, uint16_t color, uint8_t size) {
    Serial.printf("Display: Text '%s' at (%d,%d) color 0x%04X size %d\n", text, x, y, color, size);
}

static void hal_display_bitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color) {
    Serial.printf("Display: Bitmap at (%d,%d) size %dx%d color 0x%04X\n", x, y, w, h, color);
}

static void hal_display_update(void) {
    Serial.println("Display: Update");
}

// Audio HAL implementations
static bool hal_audio_play_tone(uint16_t frequency, uint32_t duration_ms) {
    Serial.printf("Audio: Play tone %dHz for %dms\n", frequency, duration_ms);
    return true;
}

static bool hal_audio_play_wav(const char* filename) {
    Serial.printf("Audio: Play WAV file '%s'\n", filename);
    return true;
}

static bool hal_audio_play_mp3(const char* filename) {
    Serial.printf("Audio: Play MP3 file '%s'\n", filename);
    return true;
}

static void hal_audio_stop(void) {
    Serial.println("Audio: Stop");
}

static void hal_audio_pause(void) {
    Serial.println("Audio: Pause");
}

static void hal_audio_resume(void) {
    Serial.println("Audio: Resume");
}

static void hal_audio_set_volume(uint8_t volume) {
    Serial.printf("Audio: Set volume to %d\n", volume);
}

static uint8_t hal_audio_get_volume(void) {
    return 50; // Default volume
}

static bool hal_audio_is_playing(void) {
    return false; // Not playing by default
}

// Storage HAL implementations
static bool hal_storage_exists(const char* path) {
    return SD.exists(path);
}

static void* hal_storage_open(const char* path, const char* mode) {
    File* file = new File();
    if (strcmp(mode, "r") == 0) {
        *file = SD.open(path, FILE_READ);
    } else if (strcmp(mode, "w") == 0) {
        *file = SD.open(path, FILE_WRITE);
    } else {
        delete file;
        return nullptr;
    }
    
    if (!*file) {
        delete file;
        return nullptr;
    }
    
    return file;
}

static void hal_storage_close(void* file) {
    if (file) {
        File* f = static_cast<File*>(file);
        f->close();
        delete f;
    }
}

static size_t hal_storage_read(void* file, void* buffer, size_t size) {
    if (!file) return 0;
    File* f = static_cast<File*>(file);
    return f->read(static_cast<uint8_t*>(buffer), size);
}

static size_t hal_storage_write(void* file, const void* buffer, size_t size) {
    if (!file) return 0;
    File* f = static_cast<File*>(file);
    return f->write(static_cast<const uint8_t*>(buffer), size);
}

static bool hal_storage_seek(void* file, long offset, int whence) {
    if (!file) return false;
    File* f = static_cast<File*>(file);
    return f->seek(offset);
}

static long hal_storage_tell(void* file) {
    if (!file) return -1;
    File* f = static_cast<File*>(file);
    return f->position();
}

static bool hal_storage_mkdir(const char* path) {
    return SD.mkdir(path);
}

static bool hal_storage_remove(const char* path) {
    return SD.remove(path);
}

static bool hal_storage_rename(const char* old_path, const char* new_path) {
    return SD.rename(old_path, new_path);
}

// Input HAL implementations
static bool hal_input_is_button_pressed(uint8_t button_id) {
    // This would interface with actual button reading code
    return false;
}

static int16_t hal_input_get_wheel_delta(void) {
    // This would interface with touch wheel code
    return 0;
}

static uint16_t hal_input_get_wheel_position(void) {
    // This would interface with touch wheel code
    return 0;
}

static uint8_t hal_input_get_touched_electrodes(void) {
    // This would interface with MPR121 code
    return 0;
}

// System HAL implementations
static uint32_t hal_system_get_time_ms(void) {
    return millis();
}

static void hal_system_delay_ms(uint32_t ms) {
    delay(ms);
}

static void hal_system_log(const char* level, const char* message) {
    Serial.printf("[%s] %s\n", level, message);
}

static uint32_t hal_system_get_free_heap(void) {
    return ESP.getFreeHeap();
}

static uint8_t hal_system_get_battery_level(void) {
    // This would interface with battery monitoring code
    return 100; // Full battery by default
}

static bool hal_system_is_charging(void) {
    // This would interface with charging detection code
    return false;
}

static void hal_system_request_sleep(void) {
    Serial.println("System: Sleep requested by plugin");
}

// =============================================================================
// Plugin Manager Implementation
// =============================================================================

static void init_hal_interfaces() {
    // Initialize display HAL
    g_display_hal.clear = hal_display_clear;
    g_display_hal.pixel = hal_display_pixel;
    g_display_hal.line = hal_display_line;
    g_display_hal.rect = hal_display_rect;
    g_display_hal.fill_rect = hal_display_fill_rect;
    g_display_hal.circle = hal_display_circle;
    g_display_hal.fill_circle = hal_display_fill_circle;
    g_display_hal.text = hal_display_text;
    g_display_hal.bitmap = hal_display_bitmap;
    g_display_hal.update = hal_display_update;
    g_display_hal.width = TFT_WIDTH;
    g_display_hal.height = TFT_HEIGHT;
    
    // Initialize audio HAL
    g_audio_hal.play_tone = hal_audio_play_tone;
    g_audio_hal.play_wav = hal_audio_play_wav;
    g_audio_hal.play_mp3 = hal_audio_play_mp3;
    g_audio_hal.stop = hal_audio_stop;
    g_audio_hal.pause = hal_audio_pause;
    g_audio_hal.resume = hal_audio_resume;
    g_audio_hal.set_volume = hal_audio_set_volume;
    g_audio_hal.get_volume = hal_audio_get_volume;
    g_audio_hal.is_playing = hal_audio_is_playing;
    
    // Initialize storage HAL
    g_storage_hal.exists = hal_storage_exists;
    g_storage_hal.open = hal_storage_open;
    g_storage_hal.close = hal_storage_close;
    g_storage_hal.read = hal_storage_read;
    g_storage_hal.write = hal_storage_write;
    g_storage_hal.seek = hal_storage_seek;
    g_storage_hal.tell = hal_storage_tell;
    g_storage_hal.mkdir = hal_storage_mkdir;
    g_storage_hal.remove = hal_storage_remove;
    g_storage_hal.rename = hal_storage_rename;
    
    // Initialize input HAL
    g_input_hal.is_button_pressed = hal_input_is_button_pressed;
    g_input_hal.get_wheel_delta = hal_input_get_wheel_delta;
    g_input_hal.get_wheel_position = hal_input_get_wheel_position;
    g_input_hal.get_touched_electrodes = hal_input_get_touched_electrodes;
    
    // Initialize system HAL
    g_system_hal.get_time_ms = hal_system_get_time_ms;
    g_system_hal.delay_ms = hal_system_delay_ms;
    g_system_hal.log = hal_system_log;
    g_system_hal.get_free_heap = hal_system_get_free_heap;
    g_system_hal.get_battery_level = hal_system_get_battery_level;
    g_system_hal.is_charging = hal_system_is_charging;
    g_system_hal.request_sleep = hal_system_request_sleep;
    
    // Initialize main HAL structure
    g_hal.display = &g_display_hal;
    g_hal.audio = &g_audio_hal;
    g_hal.storage = &g_storage_hal;
    g_hal.input = &g_input_hal;
    g_hal.system = &g_system_hal;
}

bool plugin_manager_init(void) {
    if (g_plugin_manager_initialized) {
        return true;
    }
    
    // Initialize plugin runtime array
    memset(g_loaded_plugins, 0, sizeof(g_loaded_plugins));
    
    // Initialize HAL interfaces
    init_hal_interfaces();
    
    // Create plugins directory if it doesn't exist
    if (!SD.exists("/Apps")) {
        SD.mkdir("/Apps");
    }
    
    g_plugin_manager_initialized = true;
    Serial.println("Plugin manager initialized");
    
    return true;
}

static bool load_plugin_manifest(const char* plugin_path, plugin_manifest_t* manifest) {
    if (!plugin_path || !manifest) return false;
    
    char manifest_path[300];
    snprintf(manifest_path, sizeof(manifest_path), "%s/%s", plugin_path, PLUGIN_MANIFEST_FILE);
    
    if (!SD.exists(manifest_path)) {
        Serial.printf("Plugin manifest not found: %s\n", manifest_path);
        return false;
    }
    
    File manifest_file = SD.open(manifest_path, FILE_READ);
    if (!manifest_file) {
        Serial.printf("Failed to open manifest file: %s\n", manifest_path);
        return false;
    }
    
    // Parse JSON manifest
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, manifest_file);
    manifest_file.close();
    
    if (error) {
        Serial.printf("Failed to parse manifest JSON: %s\n", error.c_str());
        return false;
    }
    
    // Extract manifest data
    strncpy(manifest->name, doc["name"] | "Unknown", PLUGIN_NAME_MAX_LENGTH - 1);
    strncpy(manifest->version, doc["version"] | "1.0.0", PLUGIN_VERSION_MAX_LENGTH - 1);
    strncpy(manifest->author, doc["author"] | "Unknown", PLUGIN_AUTHOR_MAX_LENGTH - 1);
    strncpy(manifest->description, doc["description"] | "", PLUGIN_DESC_MAX_LENGTH - 1);
    
    manifest->category = static_cast<plugin_category_t>(doc["category"] | PLUGIN_CATEGORY_OTHER);
    manifest->compatibility_flags = doc["compatibility_flags"] | 0;
    manifest->memory_required = doc["memory_required"] | 0;
    
    // Parse API version
    const char* api_version = doc["api_version"] | "1.0.0";
    // Simple version parsing (would need more robust implementation)
    manifest->api_version = (1 << 16) | (0 << 8) | 0; // Default to 1.0.0
    
    // Function pointers would be loaded from the plugin binary
    // For now, set to NULL (would be populated during dynamic loading)
    manifest->init = nullptr;
    manifest->run = nullptr;
    manifest->cleanup = nullptr;
    manifest->event_handler = nullptr;
    manifest->suspend = nullptr;
    manifest->resume = nullptr;
    
    // Icon data (optional)
    manifest->icon_data = nullptr;
    manifest->icon_size = 0;
    manifest->icon_width = 0;
    manifest->icon_height = 0;
    
    return true;
}

static bool validate_plugin_compatibility(const plugin_manifest_t* manifest) {
    if (!manifest) return false;
    
    // Check API version compatibility
    uint32_t current_api = (PLUGIN_API_VERSION_MAJOR << 16) | 
                          (PLUGIN_API_VERSION_MINOR << 8) | 
                          PLUGIN_API_VERSION_PATCH;
    
    if (manifest->api_version > current_api) {
        Serial.printf("Plugin requires newer API version: 0x%08X > 0x%08X\n", 
                      manifest->api_version, current_api);
        return false;
    }
    
    // Check hardware compatibility
    uint32_t available_features = 0;
    #if HW_FEATURE_TOUCH_WHEEL
    available_features |= PLUGIN_COMPAT_TOUCH_WHEEL;
    #endif
    #if HW_FEATURE_AUDIO_DAC
    available_features |= PLUGIN_COMPAT_AUDIO_DAC;
    #endif
    #if HW_FEATURE_TFT_DISPLAY
    available_features |= PLUGIN_COMPAT_TFT_DISPLAY;
    #endif
    #if HW_FEATURE_SD_CARD
    available_features |= PLUGIN_COMPAT_SD_CARD;
    #endif
    #if HW_FEATURE_PHYSICAL_BTNS
    available_features |= PLUGIN_COMPAT_PHYSICAL_BTNS;
    #endif
    
    if ((manifest->compatibility_flags & available_features) != manifest->compatibility_flags) {
        Serial.printf("Plugin requires unavailable hardware features: 0x%08X\n", 
                      manifest->compatibility_flags & ~available_features);
        return false;
    }
    
    // Check memory requirements
    if (manifest->memory_required > ESP.getFreeHeap()) {
        Serial.printf("Plugin requires more memory than available: %d > %d\n", 
                      manifest->memory_required, ESP.getFreeHeap());
        return false;
    }
    
    return true;
}

static plugin_runtime_t* find_plugin_by_id(uint32_t plugin_id) {
    for (int i = 0; i < MAX_LOADED_PLUGINS; i++) {
        if (g_loaded_plugins[i].active && g_loaded_plugins[i].context.plugin_id == plugin_id) {
            return &g_loaded_plugins[i];
        }
    }
    return nullptr;
}

uint32_t plugin_manager_load(const char* plugin_path) {
    if (!g_plugin_manager_initialized || !plugin_path) {
        return 0;
    }
    
    // Find empty slot
    plugin_runtime_t* slot = nullptr;
    for (int i = 0; i < MAX_LOADED_PLUGINS; i++) {
        if (!g_loaded_plugins[i].active) {
            slot = &g_loaded_plugins[i];
            break;
        }
    }
    
    if (!slot) {
        Serial.println("No free plugin slots available");
        return 0;
    }
    
    // Load and validate manifest
    plugin_manifest_t manifest;
    if (!load_plugin_manifest(plugin_path, &manifest)) {
        return 0;
    }
    
    if (!validate_plugin_compatibility(&manifest)) {
        return 0;
    }
    
    // Initialize plugin context
    memset(slot, 0, sizeof(plugin_runtime_t));
    slot->active = true;
    slot->context.plugin_id = g_next_plugin_id++;
    slot->context.state = PLUGIN_STATE_LOADED;
    slot->context.manifest = &manifest; // Note: This needs to be allocated properly
    slot->load_time = millis();
    strncpy(slot->plugin_path, plugin_path, sizeof(slot->plugin_path) - 1);
    
    Serial.printf("Plugin loaded: %s (ID: %d)\n", manifest.name, slot->context.plugin_id);
    
    return slot->context.plugin_id;
}

bool plugin_manager_unload(uint32_t plugin_id) {
    plugin_runtime_t* plugin = find_plugin_by_id(plugin_id);
    if (!plugin) {
        return false;
    }
    
    // Stop plugin if running
    if (plugin->context.state == PLUGIN_STATE_RUNNING) {
        plugin_manager_stop(plugin_id);
    }
    
    // Cleanup plugin resources
    if (plugin->context.manifest && plugin->context.manifest->cleanup) {
        plugin->context.manifest->cleanup(&plugin->context);
    }
    
    // Free plugin slot
    memset(plugin, 0, sizeof(plugin_runtime_t));
    
    Serial.printf("Plugin unloaded: ID %d\n", plugin_id);
    return true;
}

bool plugin_manager_run(uint32_t plugin_id) {
    plugin_runtime_t* plugin = find_plugin_by_id(plugin_id);
    if (!plugin || plugin->context.state != PLUGIN_STATE_LOADED) {
        return false;
    }
    
    // Stop currently running plugin
    if (g_current_running_plugin != 0) {
        plugin_manager_stop(g_current_running_plugin);
    }
    
    // Initialize plugin if needed
    if (plugin->context.manifest && plugin->context.manifest->init) {
        if (!plugin->context.manifest->init(&plugin->context)) {
            Serial.printf("Plugin initialization failed: ID %d\n", plugin_id);
            return false;
        }
    }
    
    plugin->context.state = PLUGIN_STATE_RUNNING;
    g_current_running_plugin = plugin_id;
    
    Serial.printf("Plugin running: ID %d\n", plugin_id);
    return true;
}

bool plugin_manager_stop(uint32_t plugin_id) {
    plugin_runtime_t* plugin = find_plugin_by_id(plugin_id);
    if (!plugin || plugin->context.state != PLUGIN_STATE_RUNNING) {
        return false;
    }
    
    plugin->context.state = PLUGIN_STATE_LOADED;
    if (g_current_running_plugin == plugin_id) {
        g_current_running_plugin = 0;
    }
    
    Serial.printf("Plugin stopped: ID %d\n", plugin_id);
    return true;
}

bool plugin_manager_suspend(uint32_t plugin_id) {
    plugin_runtime_t* plugin = find_plugin_by_id(plugin_id);
    if (!plugin || plugin->context.state != PLUGIN_STATE_RUNNING) {
        return false;
    }
    
    if (plugin->context.manifest && plugin->context.manifest->suspend) {
        plugin->context.manifest->suspend(&plugin->context);
    }
    
    plugin->context.state = PLUGIN_STATE_SUSPENDED;
    
    Serial.printf("Plugin suspended: ID %d\n", plugin_id);
    return true;
}

bool plugin_manager_resume(uint32_t plugin_id) {
    plugin_runtime_t* plugin = find_plugin_by_id(plugin_id);
    if (!plugin || plugin->context.state != PLUGIN_STATE_SUSPENDED) {
        return false;
    }
    
    if (plugin->context.manifest && plugin->context.manifest->resume) {
        plugin->context.manifest->resume(&plugin->context);
    }
    
    plugin->context.state = PLUGIN_STATE_RUNNING;
    
    Serial.printf("Plugin resumed: ID %d\n", plugin_id);
    return true;
}

bool plugin_manager_send_event(uint32_t plugin_id, uint32_t event, void* data) {
    plugin_runtime_t* plugin = find_plugin_by_id(plugin_id);
    if (!plugin || plugin->context.state != PLUGIN_STATE_RUNNING) {
        return false;
    }
    
    if (plugin->context.manifest && plugin->context.manifest->event_handler) {
        return plugin->context.manifest->event_handler(&plugin->context, event, data);
    }
    
    return false;
}

plugin_context_t* plugin_manager_get_context(uint32_t plugin_id) {
    plugin_runtime_t* plugin = find_plugin_by_id(plugin_id);
    return plugin ? &plugin->context : nullptr;
}

uint32_t plugin_manager_get_loaded_plugins(uint32_t* plugin_ids, uint32_t max_count) {
    uint32_t count = 0;
    
    for (int i = 0; i < MAX_LOADED_PLUGINS && count < max_count; i++) {
        if (g_loaded_plugins[i].active) {
            plugin_ids[count++] = g_loaded_plugins[i].context.plugin_id;
        }
    }
    
    return count;
}

uint32_t plugin_manager_scan_plugins(char plugin_paths[][256], uint32_t max_count) {
    uint32_t count = 0;
    
    File apps_dir = SD.open("/Apps");
    if (!apps_dir) {
        Serial.println("Failed to open /Apps directory");
        return 0;
    }
    
    File entry;
    while ((entry = apps_dir.openNextFile()) && count < max_count) {
        if (entry.isDirectory()) {
            char manifest_path[300];
            snprintf(manifest_path, sizeof(manifest_path), "/Apps/%s/%s", 
                     entry.name(), PLUGIN_MANIFEST_FILE);
            
            if (SD.exists(manifest_path)) {
                snprintf(plugin_paths[count], 256, "/Apps/%s", entry.name());
                count++;
            }
        }
        entry.close();
    }
    apps_dir.close();
    
    Serial.printf("Found %d plugins in /Apps\n", count);
    return count;
}

const plugin_hal_t* plugin_get_hal(void) {
    return &g_hal;
}

// =============================================================================
// Plugin Update Loop (called from main loop)
// =============================================================================

void plugin_manager_update() {
    if (!g_plugin_manager_initialized || g_current_running_plugin == 0) {
        return;
    }
    
    plugin_runtime_t* plugin = find_plugin_by_id(g_current_running_plugin);
    if (!plugin || plugin->context.state != PLUGIN_STATE_RUNNING) {
        return;
    }
    
    // Call plugin's run function
    if (plugin->context.manifest && plugin->context.manifest->run) {
        plugin->context.manifest->run(&plugin->context);
    }
    
    // Update runtime statistics
    plugin->context.last_activity = millis();
    plugin->context.memory_used = ESP.getFreeHeap(); // Simplified tracking
}

