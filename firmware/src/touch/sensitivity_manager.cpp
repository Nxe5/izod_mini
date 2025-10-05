/*
 * Touch Sensitivity Manager
 * Manages MPR121 touch sensitivity levels and per-electrode configuration
 */

#include "touch_config.h"
#include "hardware_config.h"
#include <Arduino.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_MPR121.h>

// Global touch configuration
static touch_sensitivity_config_t g_touch_config;
static touch_wheel_config_t g_wheel_config;
static Adafruit_MPR121 g_mpr121;
static Preferences g_preferences;

// Sensitivity level threshold tables
static const struct {
    uint8_t touch_threshold;
    uint8_t release_threshold;
} sensitivity_levels[TOUCH_SENSITIVITY_MAX] = {
    TOUCH_THRESHOLDS_LEVEL_1,  // Level 1: Least sensitive
    TOUCH_THRESHOLDS_LEVEL_2,  // Level 2: Low sensitivity
    TOUCH_THRESHOLDS_LEVEL_3,  // Level 3: Medium sensitivity
    TOUCH_THRESHOLDS_LEVEL_4,  // Level 4: High sensitivity
    TOUCH_THRESHOLDS_LEVEL_5   // Level 5: Most sensitive
};

// Special thresholds for small pad (electrode 11)
static const struct {
    uint8_t touch_threshold;
    uint8_t release_threshold;
} small_pad_levels[TOUCH_SENSITIVITY_MAX] = {
    TOUCH_SMALL_PAD_THRESHOLDS_LEVEL_1,
    TOUCH_SMALL_PAD_THRESHOLDS_LEVEL_2,
    TOUCH_SMALL_PAD_THRESHOLDS_LEVEL_3,
    TOUCH_SMALL_PAD_THRESHOLDS_LEVEL_4,
    TOUCH_SMALL_PAD_THRESHOLDS_LEVEL_5
};

// Sensitivity level names
static const char* sensitivity_names[] = {
    "Very Low",    // Level 1
    "Low",         // Level 2
    "Medium",      // Level 3
    "High",        // Level 4
    "Very High"    // Level 5
};

// =============================================================================
// Configuration Management Functions
// =============================================================================

void touch_config_init_defaults(touch_sensitivity_config_t* config) {
    if (!config) return;
    
    // Set default configuration
    config->global_level = TOUCH_SENSITIVITY_DEFAULT;
    config->custom_per_pad = false;
    config->small_pad_compensation = true;
    config->debounce_count = 2;
    config->filter_config = 0x04;
    config->auto_calibration = true;
    config->calibration_interval_ms = 30000;
    
    // Apply default thresholds based on global level
    touch_config_set_sensitivity_level(config, config->global_level);
}

bool touch_config_set_sensitivity_level(touch_sensitivity_config_t* config, uint8_t level) {
    if (!config || level < TOUCH_SENSITIVITY_MIN || level > TOUCH_SENSITIVITY_MAX) {
        return false;
    }
    
    config->global_level = level;
    uint8_t level_index = level - 1; // Convert to 0-based index
    
    // Set thresholds for all electrodes
    for (int i = 0; i < TOUCH_ELECTRODE_COUNT; i++) {
        if (i == TOUCH_SMALL_PAD_ELECTRODE && config->small_pad_compensation) {
            // Use special thresholds for small pad
            config->touch_threshold[i] = small_pad_levels[level_index].touch_threshold;
            config->release_threshold[i] = small_pad_levels[level_index].release_threshold;
        } else {
            // Use standard thresholds
            config->touch_threshold[i] = sensitivity_levels[level_index].touch_threshold;
            config->release_threshold[i] = sensitivity_levels[level_index].release_threshold;
        }
    }
    
    return true;
}

uint8_t touch_config_get_sensitivity_level(const touch_sensitivity_config_t* config) {
    if (!config) return TOUCH_SENSITIVITY_DEFAULT;
    return config->global_level;
}

bool touch_config_set_electrode_threshold(touch_sensitivity_config_t* config, 
                                         uint8_t electrode,
                                         uint8_t touch_threshold, 
                                         uint8_t release_threshold) {
    if (!config || electrode >= TOUCH_ELECTRODE_COUNT) {
        return false;
    }
    
    // Validate threshold values (MPR121 uses 0-255 range)
    if (touch_threshold == 0 || release_threshold == 0 || 
        touch_threshold <= release_threshold) {
        return false;
    }
    
    config->touch_threshold[electrode] = touch_threshold;
    config->release_threshold[electrode] = release_threshold;
    config->custom_per_pad = true; // Enable custom mode when individual thresholds are set
    
    return true;
}

void touch_config_set_custom_per_pad(touch_sensitivity_config_t* config, bool enable) {
    if (!config) return;
    config->custom_per_pad = enable;
}

void touch_config_apply_small_pad_compensation(touch_sensitivity_config_t* config) {
    if (!config) return;
    
    config->small_pad_compensation = true;
    
    // Reapply sensitivity level to update small pad thresholds
    touch_config_set_sensitivity_level(config, config->global_level);
}

bool touch_config_validate(const touch_sensitivity_config_t* config) {
    if (!config) return false;
    
    // Validate global level
    if (config->global_level < TOUCH_SENSITIVITY_MIN || 
        config->global_level > TOUCH_SENSITIVITY_MAX) {
        return false;
    }
    
    // Validate individual thresholds
    for (int i = 0; i < TOUCH_ELECTRODE_COUNT; i++) {
        if (config->touch_threshold[i] == 0 || 
            config->release_threshold[i] == 0 ||
            config->touch_threshold[i] <= config->release_threshold[i]) {
            return false;
        }
    }
    
    return true;
}

bool touch_config_load_from_nvs(touch_sensitivity_config_t* config) {
    if (!config) return false;
    
    if (!g_preferences.begin("touch_config", true)) { // Read-only mode
        Serial.println("Failed to open touch_config namespace");
        return false;
    }
    
    // Load configuration from NVS
    config->global_level = g_preferences.getUChar("global_level", TOUCH_SENSITIVITY_DEFAULT);
    config->custom_per_pad = g_preferences.getBool("custom_per_pad", false);
    config->small_pad_compensation = g_preferences.getBool("small_pad_comp", true);
    config->debounce_count = g_preferences.getUChar("debounce_count", 2);
    config->filter_config = g_preferences.getUChar("filter_config", 0x04);
    config->auto_calibration = g_preferences.getBool("auto_cal", true);
    config->calibration_interval_ms = g_preferences.getULong("cal_interval", 30000);
    
    // Load individual thresholds
    for (int i = 0; i < TOUCH_ELECTRODE_COUNT; i++) {
        char key[16];
        snprintf(key, sizeof(key), "touch_th_%d", i);
        config->touch_threshold[i] = g_preferences.getUChar(key, 8);
        
        snprintf(key, sizeof(key), "release_th_%d", i);
        config->release_threshold[i] = g_preferences.getUChar(key, 4);
    }
    
    g_preferences.end();
    
    // Validate loaded configuration
    if (!touch_config_validate(config)) {
        Serial.println("Invalid touch configuration loaded, using defaults");
        touch_config_init_defaults(config);
        return false;
    }
    
    Serial.printf("Touch config loaded: level=%d, custom=%s\n", 
                  config->global_level, config->custom_per_pad ? "yes" : "no");
    return true;
}

bool touch_config_save_to_nvs(const touch_sensitivity_config_t* config) {
    if (!config || !touch_config_validate(config)) {
        return false;
    }
    
    if (!g_preferences.begin("touch_config", false)) { // Read-write mode
        Serial.println("Failed to open touch_config namespace for writing");
        return false;
    }
    
    // Save configuration to NVS
    g_preferences.putUChar("global_level", config->global_level);
    g_preferences.putBool("custom_per_pad", config->custom_per_pad);
    g_preferences.putBool("small_pad_comp", config->small_pad_compensation);
    g_preferences.putUChar("debounce_count", config->debounce_count);
    g_preferences.putUChar("filter_config", config->filter_config);
    g_preferences.putBool("auto_cal", config->auto_calibration);
    g_preferences.putULong("cal_interval", config->calibration_interval_ms);
    
    // Save individual thresholds
    for (int i = 0; i < TOUCH_ELECTRODE_COUNT; i++) {
        char key[16];
        snprintf(key, sizeof(key), "touch_th_%d", i);
        g_preferences.putUChar(key, config->touch_threshold[i]);
        
        snprintf(key, sizeof(key), "release_th_%d", i);
        g_preferences.putUChar(key, config->release_threshold[i]);
    }
    
    g_preferences.end();
    
    Serial.printf("Touch config saved: level=%d, custom=%s\n", 
                  config->global_level, config->custom_per_pad ? "yes" : "no");
    return true;
}

const char* touch_config_get_sensitivity_name(uint8_t level) {
    if (level < TOUCH_SENSITIVITY_MIN || level > TOUCH_SENSITIVITY_MAX) {
        return "Invalid";
    }
    return sensitivity_names[level - 1];
}

// =============================================================================
// Wheel Configuration Functions
// =============================================================================

void touch_wheel_config_init_defaults(touch_wheel_config_t* config) {
    if (!config) return;
    
    // Set default electrode order (0-11 in sequence)
    for (int i = 0; i < TOUCH_ELECTRODE_COUNT; i++) {
        config->electrode_order[i] = i;
    }
    
    config->invert_direction = false;
    config->center_of_mass_threshold = 3;
    config->position_hysteresis = 2;
    config->enable_interpolation = true;
    config->scroll_acceleration = 1;
}

void touch_wheel_config_set_electrode_order(touch_wheel_config_t* config, 
                                           const uint8_t* order) {
    if (!config || !order) return;
    
    for (int i = 0; i < TOUCH_ELECTRODE_COUNT; i++) {
        config->electrode_order[i] = order[i];
    }
}

void touch_wheel_config_set_invert_direction(touch_wheel_config_t* config, bool invert) {
    if (!config) return;
    config->invert_direction = invert;
}

// =============================================================================
// MPR121 Hardware Interface Functions
// =============================================================================

bool touch_sensitivity_manager_init() {
    // Initialize I2C
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_FREQUENCY);
    
    // Initialize MPR121
    if (!g_mpr121.begin(MPR121_I2C_ADDR, &Wire)) {
        Serial.println("MPR121 not found, check wiring");
        return false;
    }
    
    // Load configuration from NVS or use defaults
    if (!touch_config_load_from_nvs(&g_touch_config)) {
        touch_config_init_defaults(&g_touch_config);
        // Save default configuration
        touch_config_save_to_nvs(&g_touch_config);
    }
    
    // Initialize wheel configuration
    touch_wheel_config_init_defaults(&g_wheel_config);
    
    // Apply configuration to MPR121
    touch_sensitivity_manager_apply_config();
    
    Serial.printf("Touch sensitivity manager initialized: level=%d (%s)\n",
                  g_touch_config.global_level,
                  touch_config_get_sensitivity_name(g_touch_config.global_level));
    
    return true;
}

bool touch_sensitivity_manager_apply_config() {
    if (!touch_config_validate(&g_touch_config)) {
        Serial.println("Invalid touch configuration, cannot apply");
        return false;
    }
    
    // Apply thresholds to MPR121
    for (int i = 0; i < TOUCH_ELECTRODE_COUNT; i++) {
        g_mpr121.setThreshold(i, g_touch_config.touch_threshold[i], 
                             g_touch_config.release_threshold[i]);
    }
    
    // Configure debounce and filter settings
    // Note: These would require direct register access for full MPR121 configuration
    // The Adafruit library provides limited configuration options
    
    Serial.println("Touch configuration applied to MPR121");
    return true;
}

touch_sensitivity_config_t* touch_sensitivity_manager_get_config() {
    return &g_touch_config;
}

touch_wheel_config_t* touch_wheel_manager_get_config() {
    return &g_wheel_config;
}

bool touch_sensitivity_manager_set_level(uint8_t level) {
    if (!touch_config_set_sensitivity_level(&g_touch_config, level)) {
        return false;
    }
    
    // Apply new configuration to hardware
    touch_sensitivity_manager_apply_config();
    
    // Save to NVS
    touch_config_save_to_nvs(&g_touch_config);
    
    Serial.printf("Touch sensitivity changed to level %d (%s)\n",
                  level, touch_config_get_sensitivity_name(level));
    
    return true;
}

uint8_t touch_sensitivity_manager_get_level() {
    return g_touch_config.global_level;
}

bool touch_sensitivity_manager_set_electrode_threshold(uint8_t electrode,
                                                      uint8_t touch_threshold,
                                                      uint8_t release_threshold) {
    if (!touch_config_set_electrode_threshold(&g_touch_config, electrode,
                                             touch_threshold, release_threshold)) {
        return false;
    }
    
    // Apply single electrode threshold to hardware
    g_mpr121.setThreshold(electrode, touch_threshold, release_threshold);
    
    // Save to NVS
    touch_config_save_to_nvs(&g_touch_config);
    
    Serial.printf("Electrode %d thresholds set: touch=%d, release=%d\n",
                  electrode, touch_threshold, release_threshold);
    
    return true;
}

void touch_sensitivity_manager_print_status() {
    Serial.println("=== Touch Sensitivity Status ===");
    Serial.printf("Global Level: %d (%s)\n", 
                  g_touch_config.global_level,
                  touch_config_get_sensitivity_name(g_touch_config.global_level));
    Serial.printf("Custom Per-Pad: %s\n", g_touch_config.custom_per_pad ? "Enabled" : "Disabled");
    Serial.printf("Small Pad Compensation: %s\n", g_touch_config.small_pad_compensation ? "Enabled" : "Disabled");
    Serial.printf("Auto Calibration: %s\n", g_touch_config.auto_calibration ? "Enabled" : "Disabled");
    
    Serial.println("Electrode Thresholds:");
    for (int i = 0; i < TOUCH_ELECTRODE_COUNT; i++) {
        Serial.printf("  E%02d: touch=%3d, release=%3d%s\n", 
                      i, 
                      g_touch_config.touch_threshold[i],
                      g_touch_config.release_threshold[i],
                      (i == TOUCH_SMALL_PAD_ELECTRODE) ? " (small pad)" : "");
    }
    Serial.println("================================");
}

