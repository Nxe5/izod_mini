/*
 * Touch Calibration System
 * Runtime calibration and monitoring for MPR121 touch controller
 */

#include "touch_config.h"
#include "hardware_config.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPR121.h>

// External MPR121 instance (defined in sensitivity_manager.cpp)
extern Adafruit_MPR121 g_mpr121;

// Global calibration data
static touch_calibration_data_t g_calibration_data;
static bool g_calibration_initialized = false;
static uint32_t g_last_auto_calibration = 0;

// Calibration parameters
#define CALIBRATION_SAMPLES         10      // Number of samples for baseline calculation
#define CALIBRATION_SETTLE_TIME_MS  100     // Time to settle before sampling
#define BASELINE_DRIFT_THRESHOLD    50      // Threshold for baseline drift detection
#define TOUCH_DETECTION_THRESHOLD   10      // Minimum delta for touch detection
#define ELECTRODE_DISABLE_THRESHOLD 1000    // Threshold to disable problematic electrodes

// =============================================================================
// Internal Helper Functions
// =============================================================================

static uint16_t read_electrode_baseline(uint8_t electrode) {
    if (electrode >= TOUCH_ELECTRODE_COUNT) return 0;
    
    // Read baseline value from MPR121
    // Note: This requires direct register access as Adafruit library doesn't expose this
    Wire.beginTransmission(MPR121_I2C_ADDR);
    Wire.write(0x1E + electrode); // Baseline register for electrode
    Wire.endTransmission();
    
    Wire.requestFrom(MPR121_I2C_ADDR, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

static uint16_t read_electrode_filtered_data(uint8_t electrode) {
    if (electrode >= TOUCH_ELECTRODE_COUNT) return 0;
    
    // Read filtered data from MPR121
    Wire.beginTransmission(MPR121_I2C_ADDR);
    Wire.write(0x04 + (electrode * 2)); // Filtered data register (LSB)
    Wire.endTransmission();
    
    Wire.requestFrom(MPR121_I2C_ADDR, 2);
    if (Wire.available() >= 2) {
        uint16_t lsb = Wire.read();
        uint16_t msb = Wire.read();
        return (msb << 8) | lsb;
    }
    return 0;
}

static void calculate_touch_deltas() {
    for (int i = 0; i < TOUCH_ELECTRODE_COUNT; i++) {
        uint16_t filtered = g_calibration_data.filtered_data[i];
        uint16_t baseline = g_calibration_data.baseline[i];
        
        if (baseline > filtered) {
            g_calibration_data.touch_delta[i] = baseline - filtered;
        } else {
            g_calibration_data.touch_delta[i] = 0;
        }
    }
}

static bool is_electrode_healthy(uint8_t electrode) {
    if (electrode >= TOUCH_ELECTRODE_COUNT) return false;
    
    // Check if electrode readings are within reasonable range
    uint16_t baseline = g_calibration_data.baseline[electrode];
    uint16_t filtered = g_calibration_data.filtered_data[electrode];
    
    // Electrode is unhealthy if baseline is too low/high or if there's excessive noise
    if (baseline < 50 || baseline > 1000) {
        return false;
    }
    
    // Check for excessive noise (large difference between consecutive readings)
    static uint16_t last_filtered[TOUCH_ELECTRODE_COUNT] = {0};
    uint16_t noise = abs((int16_t)filtered - (int16_t)last_filtered[electrode]);
    last_filtered[electrode] = filtered;
    
    if (noise > 100) { // Excessive noise threshold
        return false;
    }
    
    return true;
}

// =============================================================================
// Public Calibration Functions
// =============================================================================

bool touch_calibration_init() {
    // Initialize calibration data structure
    memset(&g_calibration_data, 0, sizeof(g_calibration_data));
    
    // Enable all electrodes initially
    for (int i = 0; i < TOUCH_ELECTRODE_COUNT; i++) {
        g_calibration_data.electrode_enabled[i] = true;
    }
    
    g_calibration_data.calibration_needed = true;
    g_calibration_data.last_calibration_time = millis();
    g_last_auto_calibration = millis();
    
    g_calibration_initialized = true;
    
    Serial.println("Touch calibration system initialized");
    return true;
}

bool touch_calibration_perform_baseline() {
    if (!g_calibration_initialized) {
        Serial.println("Calibration system not initialized");
        return false;
    }
    
    Serial.println("Performing baseline calibration...");
    
    // Wait for system to settle
    delay(CALIBRATION_SETTLE_TIME_MS);
    
    // Clear existing baseline data
    memset(g_calibration_data.baseline, 0, sizeof(g_calibration_data.baseline));
    
    // Collect multiple samples for each electrode
    for (int sample = 0; sample < CALIBRATION_SAMPLES; sample++) {
        for (int electrode = 0; electrode < TOUCH_ELECTRODE_COUNT; electrode++) {
            if (!g_calibration_data.electrode_enabled[electrode]) continue;
            
            uint16_t reading = read_electrode_baseline(electrode);
            g_calibration_data.baseline[electrode] += reading;
        }
        delay(10); // Small delay between samples
    }
    
    // Calculate average baseline for each electrode
    for (int electrode = 0; electrode < TOUCH_ELECTRODE_COUNT; electrode++) {
        if (g_calibration_data.electrode_enabled[electrode]) {
            g_calibration_data.baseline[electrode] /= CALIBRATION_SAMPLES;
        }
    }
    
    // Validate baseline readings and disable problematic electrodes
    for (int electrode = 0; electrode < TOUCH_ELECTRODE_COUNT; electrode++) {
        if (!is_electrode_healthy(electrode)) {
            g_calibration_data.electrode_enabled[electrode] = false;
            Serial.printf("Electrode %d disabled due to poor baseline reading\n", electrode);
        }
    }
    
    g_calibration_data.calibration_needed = false;
    g_calibration_data.last_calibration_time = millis();
    
    Serial.println("Baseline calibration completed");
    touch_calibration_print_status();
    
    return true;
}

void touch_calibration_update() {
    if (!g_calibration_initialized) return;
    
    // Read current filtered data for all electrodes
    for (int i = 0; i < TOUCH_ELECTRODE_COUNT; i++) {
        if (g_calibration_data.electrode_enabled[i]) {
            g_calibration_data.filtered_data[i] = read_electrode_filtered_data(i);
        }
    }
    
    // Calculate touch deltas
    calculate_touch_deltas();
    
    // Check for baseline drift
    bool drift_detected = false;
    for (int i = 0; i < TOUCH_ELECTRODE_COUNT; i++) {
        if (!g_calibration_data.electrode_enabled[i]) continue;
        
        uint16_t current_baseline = read_electrode_baseline(i);
        int16_t drift = abs((int16_t)current_baseline - (int16_t)g_calibration_data.baseline[i]);
        
        if (drift > BASELINE_DRIFT_THRESHOLD) {
            drift_detected = true;
            Serial.printf("Baseline drift detected on electrode %d: %d\n", i, drift);
        }
    }
    
    // Trigger recalibration if drift detected
    if (drift_detected) {
        g_calibration_data.calibration_needed = true;
    }
    
    // Check for auto-calibration interval
    touch_sensitivity_config_t* config = touch_sensitivity_manager_get_config();
    if (config && config->auto_calibration) {
        uint32_t now = millis();
        if (now - g_last_auto_calibration > config->calibration_interval_ms) {
            Serial.println("Auto-calibration interval reached");
            g_calibration_data.calibration_needed = true;
            g_last_auto_calibration = now;
        }
    }
    
    // Perform calibration if needed
    if (g_calibration_data.calibration_needed) {
        touch_calibration_perform_baseline();
    }
}

bool touch_calibration_is_electrode_touched(uint8_t electrode) {
    if (electrode >= TOUCH_ELECTRODE_COUNT || 
        !g_calibration_data.electrode_enabled[electrode]) {
        return false;
    }
    
    // Get current touch configuration
    touch_sensitivity_config_t* config = touch_sensitivity_manager_get_config();
    if (!config) return false;
    
    // Compare delta against touch threshold
    return g_calibration_data.touch_delta[electrode] >= config->touch_threshold[electrode];
}

uint16_t touch_calibration_get_electrode_delta(uint8_t electrode) {
    if (electrode >= TOUCH_ELECTRODE_COUNT) return 0;
    return g_calibration_data.touch_delta[electrode];
}

uint16_t touch_calibration_get_electrode_baseline(uint8_t electrode) {
    if (electrode >= TOUCH_ELECTRODE_COUNT) return 0;
    return g_calibration_data.baseline[electrode];
}

uint16_t touch_calibration_get_electrode_filtered(uint8_t electrode) {
    if (electrode >= TOUCH_ELECTRODE_COUNT) return 0;
    return g_calibration_data.filtered_data[electrode];
}

bool touch_calibration_is_electrode_enabled(uint8_t electrode) {
    if (electrode >= TOUCH_ELECTRODE_COUNT) return false;
    return g_calibration_data.electrode_enabled[electrode];
}

void touch_calibration_enable_electrode(uint8_t electrode, bool enable) {
    if (electrode >= TOUCH_ELECTRODE_COUNT) return;
    
    g_calibration_data.electrode_enabled[electrode] = enable;
    
    if (enable) {
        // Re-enable electrode and trigger recalibration
        g_calibration_data.calibration_needed = true;
        Serial.printf("Electrode %d re-enabled\n", electrode);
    } else {
        Serial.printf("Electrode %d disabled\n", electrode);
    }
}

void touch_calibration_force_recalibration() {
    g_calibration_data.calibration_needed = true;
    Serial.println("Forced recalibration requested");
}

touch_calibration_data_t* touch_calibration_get_data() {
    return &g_calibration_data;
}

void touch_calibration_print_status() {
    if (!g_calibration_initialized) {
        Serial.println("Touch calibration not initialized");
        return;
    }
    
    Serial.println("=== Touch Calibration Status ===");
    Serial.printf("Last Calibration: %lu ms ago\n", 
                  millis() - g_calibration_data.last_calibration_time);
    Serial.printf("Calibration Needed: %s\n", 
                  g_calibration_data.calibration_needed ? "Yes" : "No");
    
    Serial.println("Electrode Status:");
    Serial.println("  ID | Enabled | Baseline | Filtered | Delta | Touched");
    Serial.println("-----|---------|----------|----------|-------|--------");
    
    for (int i = 0; i < TOUCH_ELECTRODE_COUNT; i++) {
        Serial.printf("  %2d |    %s    |   %4d   |   %4d   | %4d  |   %s\n",
                      i,
                      g_calibration_data.electrode_enabled[i] ? "Y" : "N",
                      g_calibration_data.baseline[i],
                      g_calibration_data.filtered_data[i],
                      g_calibration_data.touch_delta[i],
                      touch_calibration_is_electrode_touched(i) ? "Y" : "N");
    }
    Serial.println("================================");
}

// =============================================================================
// Advanced Calibration Functions
// =============================================================================

bool touch_calibration_auto_tune_sensitivity() {
    Serial.println("Starting auto-tune sensitivity calibration...");
    
    // This function would implement automatic sensitivity tuning
    // by analyzing touch patterns and adjusting thresholds accordingly
    
    // For now, implement a basic version that adjusts based on noise levels
    touch_sensitivity_config_t* config = touch_sensitivity_manager_get_config();
    if (!config) return false;
    
    bool changes_made = false;
    
    for (int electrode = 0; electrode < TOUCH_ELECTRODE_COUNT; electrode++) {
        if (!g_calibration_data.electrode_enabled[electrode]) continue;
        
        uint16_t baseline = g_calibration_data.baseline[electrode];
        uint16_t filtered = g_calibration_data.filtered_data[electrode];
        uint16_t noise_level = abs((int16_t)baseline - (int16_t)filtered);
        
        // Adjust thresholds based on noise level
        uint8_t recommended_touch_threshold = noise_level * 2 + 5;
        uint8_t recommended_release_threshold = noise_level + 2;
        
        // Clamp to reasonable ranges
        recommended_touch_threshold = constrain(recommended_touch_threshold, 3, 30);
        recommended_release_threshold = constrain(recommended_release_threshold, 1, 15);
        
        // Apply if significantly different from current settings
        if (abs(config->touch_threshold[electrode] - recommended_touch_threshold) > 2) {
            config->touch_threshold[electrode] = recommended_touch_threshold;
            config->release_threshold[electrode] = recommended_release_threshold;
            changes_made = true;
            
            Serial.printf("Auto-tuned electrode %d: touch=%d, release=%d (noise=%d)\n",
                          electrode, recommended_touch_threshold, 
                          recommended_release_threshold, noise_level);
        }
    }
    
    if (changes_made) {
        // Apply changes to hardware and save to NVS
        touch_sensitivity_manager_apply_config();
        touch_config_save_to_nvs(config);
        Serial.println("Auto-tune completed with changes");
    } else {
        Serial.println("Auto-tune completed - no changes needed");
    }
    
    return true;
}

void touch_calibration_reset_to_factory() {
    Serial.println("Resetting touch calibration to factory defaults...");
    
    // Re-enable all electrodes
    for (int i = 0; i < TOUCH_ELECTRODE_COUNT; i++) {
        g_calibration_data.electrode_enabled[i] = true;
    }
    
    // Reset calibration data
    memset(g_calibration_data.baseline, 0, sizeof(g_calibration_data.baseline));
    memset(g_calibration_data.filtered_data, 0, sizeof(g_calibration_data.filtered_data));
    memset(g_calibration_data.touch_delta, 0, sizeof(g_calibration_data.touch_delta));
    
    g_calibration_data.calibration_needed = true;
    g_calibration_data.last_calibration_time = millis();
    
    // Reset touch configuration to defaults
    touch_sensitivity_config_t* config = touch_sensitivity_manager_get_config();
    if (config) {
        touch_config_init_defaults(config);
        touch_sensitivity_manager_apply_config();
        touch_config_save_to_nvs(config);
    }
    
    Serial.println("Factory reset completed - recalibration will occur automatically");
}

