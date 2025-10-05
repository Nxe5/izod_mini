#ifndef TOUCH_CONFIG_H
#define TOUCH_CONFIG_H

/*
 * Izod Mini Touch Configuration
 * MPR121 Touch Sensitivity and Calibration System
 * 
 * Provides 5-level sensitivity system with per-electrode customization
 * for optimal touch wheel performance.
 */

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// Touch Sensitivity Levels (1-5, where 5 is most sensitive)
// =============================================================================
#define TOUCH_SENSITIVITY_LEVEL_1    1    // Least sensitive (high thresholds)
#define TOUCH_SENSITIVITY_LEVEL_2    2    // Low sensitivity
#define TOUCH_SENSITIVITY_LEVEL_3    3    // Medium sensitivity (default)
#define TOUCH_SENSITIVITY_LEVEL_4    4    // High sensitivity
#define TOUCH_SENSITIVITY_LEVEL_5    5    // Most sensitive (low thresholds)

#define TOUCH_SENSITIVITY_DEFAULT    TOUCH_SENSITIVITY_LEVEL_3
#define TOUCH_SENSITIVITY_MIN        TOUCH_SENSITIVITY_LEVEL_1
#define TOUCH_SENSITIVITY_MAX        TOUCH_SENSITIVITY_LEVEL_5

// =============================================================================
// MPR121 Electrode Configuration
// =============================================================================
#define TOUCH_ELECTRODE_COUNT        12   // Total number of electrodes
#define TOUCH_SMALL_PAD_ELECTRODE    11   // Index of the smaller pad (0-based)

// Default threshold values for each sensitivity level
// Format: {touch_threshold, release_threshold}
#define TOUCH_THRESHOLDS_LEVEL_1     {15, 10}  // Least sensitive
#define TOUCH_THRESHOLDS_LEVEL_2     {12, 8}   // Low sensitivity
#define TOUCH_THRESHOLDS_LEVEL_3     {8, 4}    // Medium sensitivity (default)
#define TOUCH_THRESHOLDS_LEVEL_4     {6, 3}    // High sensitivity
#define TOUCH_THRESHOLDS_LEVEL_5     {4, 2}    // Most sensitive

// Special thresholds for the smaller pad (electrode 11)
#define TOUCH_SMALL_PAD_THRESHOLDS_LEVEL_1  {12, 8}   // Adjusted for smaller pad
#define TOUCH_SMALL_PAD_THRESHOLDS_LEVEL_2  {10, 6}
#define TOUCH_SMALL_PAD_THRESHOLDS_LEVEL_3  {6, 3}    // Default for small pad
#define TOUCH_SMALL_PAD_THRESHOLDS_LEVEL_4  {4, 2}
#define TOUCH_SMALL_PAD_THRESHOLDS_LEVEL_5  {3, 1}

// =============================================================================
// Touch Configuration Structure
// =============================================================================
typedef struct {
    uint8_t global_level;                    // Global sensitivity level (1-5)
    uint8_t touch_threshold[TOUCH_ELECTRODE_COUNT];   // Per-electrode touch thresholds
    uint8_t release_threshold[TOUCH_ELECTRODE_COUNT]; // Per-electrode release thresholds
    bool custom_per_pad;                     // Enable per-pad customization
    bool small_pad_compensation;             // Enable small pad compensation
    uint8_t debounce_count;                  // Touch debounce count
    uint8_t filter_config;                   // Digital filter configuration
    bool auto_calibration;                   // Enable auto-calibration
    uint32_t calibration_interval_ms;        // Auto-calibration interval
} touch_sensitivity_config_t;

// =============================================================================
// Touch Calibration Data
// =============================================================================
typedef struct {
    uint16_t baseline[TOUCH_ELECTRODE_COUNT];     // Baseline values for each electrode
    uint16_t filtered_data[TOUCH_ELECTRODE_COUNT]; // Filtered touch data
    uint16_t touch_delta[TOUCH_ELECTRODE_COUNT];   // Touch delta values
    bool electrode_enabled[TOUCH_ELECTRODE_COUNT]; // Electrode enable status
    uint32_t last_calibration_time;               // Last calibration timestamp
    bool calibration_needed;                      // Calibration needed flag
} touch_calibration_data_t;

// =============================================================================
// Touch Wheel Configuration
// =============================================================================
typedef struct {
    uint8_t electrode_order[TOUCH_ELECTRODE_COUNT]; // Physical electrode mapping
    bool invert_direction;                          // Invert wheel direction
    uint8_t center_of_mass_threshold;               // COM calculation threshold
    uint8_t position_hysteresis;                    // Position change hysteresis
    bool enable_interpolation;                      // Enable position interpolation
    uint8_t scroll_acceleration;                    // Scroll acceleration factor
} touch_wheel_config_t;

// =============================================================================
// Default Configurations
// =============================================================================

// Default sensitivity configuration
#define TOUCH_CONFIG_DEFAULT { \
    .global_level = TOUCH_SENSITIVITY_DEFAULT, \
    .touch_threshold = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 6}, \
    .release_threshold = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3}, \
    .custom_per_pad = false, \
    .small_pad_compensation = true, \
    .debounce_count = 2, \
    .filter_config = 0x04, \
    .auto_calibration = true, \
    .calibration_interval_ms = 30000 \
}

// Default wheel configuration
#define TOUCH_WHEEL_CONFIG_DEFAULT { \
    .electrode_order = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, \
    .invert_direction = false, \
    .center_of_mass_threshold = 3, \
    .position_hysteresis = 2, \
    .enable_interpolation = true, \
    .scroll_acceleration = 1 \
}

// =============================================================================
// Configuration Management Functions
// =============================================================================
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize touch configuration with defaults
 * @param config Pointer to touch configuration structure
 */
void touch_config_init_defaults(touch_sensitivity_config_t* config);

/**
 * @brief Set global sensitivity level
 * @param config Pointer to touch configuration structure
 * @param level Sensitivity level (1-5)
 * @return true if successful, false if invalid level
 */
bool touch_config_set_sensitivity_level(touch_sensitivity_config_t* config, uint8_t level);

/**
 * @brief Get current sensitivity level
 * @param config Pointer to touch configuration structure
 * @return Current sensitivity level (1-5)
 */
uint8_t touch_config_get_sensitivity_level(const touch_sensitivity_config_t* config);

/**
 * @brief Set custom threshold for specific electrode
 * @param config Pointer to touch configuration structure
 * @param electrode Electrode index (0-11)
 * @param touch_threshold Touch threshold value
 * @param release_threshold Release threshold value
 * @return true if successful, false if invalid parameters
 */
bool touch_config_set_electrode_threshold(touch_sensitivity_config_t* config, 
                                         uint8_t electrode,
                                         uint8_t touch_threshold, 
                                         uint8_t release_threshold);

/**
 * @brief Enable/disable per-pad customization
 * @param config Pointer to touch configuration structure
 * @param enable Enable per-pad customization
 */
void touch_config_set_custom_per_pad(touch_sensitivity_config_t* config, bool enable);

/**
 * @brief Apply small pad compensation
 * @param config Pointer to touch configuration structure
 */
void touch_config_apply_small_pad_compensation(touch_sensitivity_config_t* config);

/**
 * @brief Validate touch configuration
 * @param config Pointer to touch configuration structure
 * @return true if configuration is valid, false otherwise
 */
bool touch_config_validate(const touch_sensitivity_config_t* config);

/**
 * @brief Load touch configuration from NVS
 * @param config Pointer to touch configuration structure
 * @return true if loaded successfully, false if using defaults
 */
bool touch_config_load_from_nvs(touch_sensitivity_config_t* config);

/**
 * @brief Save touch configuration to NVS
 * @param config Pointer to touch configuration structure
 * @return true if saved successfully, false otherwise
 */
bool touch_config_save_to_nvs(const touch_sensitivity_config_t* config);

/**
 * @brief Initialize wheel configuration with defaults
 * @param config Pointer to wheel configuration structure
 */
void touch_wheel_config_init_defaults(touch_wheel_config_t* config);

/**
 * @brief Set electrode order for wheel mapping
 * @param config Pointer to wheel configuration structure
 * @param order Array of electrode indices in physical order
 */
void touch_wheel_config_set_electrode_order(touch_wheel_config_t* config, 
                                           const uint8_t* order);

/**
 * @brief Set wheel direction inversion
 * @param config Pointer to wheel configuration structure
 * @param invert Invert wheel direction
 */
void touch_wheel_config_set_invert_direction(touch_wheel_config_t* config, bool invert);

/**
 * @brief Get sensitivity level name string
 * @param level Sensitivity level (1-5)
 * @return String representation of sensitivity level
 */
const char* touch_config_get_sensitivity_name(uint8_t level);

#ifdef __cplusplus
}
#endif

#endif // TOUCH_CONFIG_H

