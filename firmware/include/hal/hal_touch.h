/*
 * Hardware Abstraction Layer - Touch Interface
 * Abstracts MPR121 touch operations for both ESP32 hardware and host emulation
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Touch configuration constants
#define HAL_TOUCH_MAX_ELECTRODES    12
#define HAL_TOUCH_WHEEL_ELECTRODES  8   // First 8 electrodes form the touch wheel
#define HAL_TOUCH_BUTTON_ELECTRODES 4   // Last 4 electrodes are buttons

// Touch sensitivity levels (from touch_config.h)
typedef enum {
    HAL_TOUCH_SENSITIVITY_VERY_LOW = 0,
    HAL_TOUCH_SENSITIVITY_LOW      = 1,
    HAL_TOUCH_SENSITIVITY_MEDIUM   = 2,
    HAL_TOUCH_SENSITIVITY_HIGH     = 3,
    HAL_TOUCH_SENSITIVITY_VERY_HIGH = 4
} hal_touch_sensitivity_t;

// Touch electrode states
typedef enum {
    HAL_TOUCH_STATE_RELEASED = 0,
    HAL_TOUCH_STATE_TOUCHED  = 1,
    HAL_TOUCH_STATE_PRESSED  = 2    // Stronger touch
} hal_touch_state_t;

// Touch wheel directions
typedef enum {
    HAL_TOUCH_WHEEL_NONE        = 0,
    HAL_TOUCH_WHEEL_CLOCKWISE   = 1,
    HAL_TOUCH_WHEEL_COUNTER_CW  = 2
} hal_touch_wheel_direction_t;

// Touch button IDs (matching hardware_config.h)
typedef enum {
    HAL_TOUCH_BUTTON_PREVIOUS   = 8,
    HAL_TOUCH_BUTTON_PLAY_PAUSE = 9,
    HAL_TOUCH_BUTTON_NEXT       = 10,
    HAL_TOUCH_BUTTON_MENU       = 11
} hal_touch_button_t;

// Touch electrode configuration
typedef struct {
    uint8_t touch_threshold;        // Touch detection threshold
    uint8_t release_threshold;      // Release detection threshold
    bool is_small_pad;              // Special handling for small pads
    bool enabled;                   // Electrode enabled/disabled
} hal_touch_electrode_config_t;

// Touch wheel data
typedef struct {
    bool active;                    // Wheel is being touched
    float position;                 // Position 0.0-1.0 around wheel
    float velocity;                 // Rotation velocity (-1.0 to 1.0)
    hal_touch_wheel_direction_t direction;
    uint8_t active_electrodes;      // Bitmask of active electrodes
} hal_touch_wheel_data_t;

// Touch button data
typedef struct {
    hal_touch_state_t state;        // Current button state
    uint32_t press_time;            // Time when pressed (ms)
    uint32_t hold_time;             // How long held (ms)
    bool long_press;                // Long press detected
} hal_touch_button_data_t;

// Complete touch data structure
typedef struct {
    uint16_t raw_data[HAL_TOUCH_MAX_ELECTRODES];        // Raw electrode readings
    uint16_t filtered_data[HAL_TOUCH_MAX_ELECTRODES];   // Filtered readings
    uint16_t baseline[HAL_TOUCH_MAX_ELECTRODES];        // Baseline values
    bool touched[HAL_TOUCH_MAX_ELECTRODES];             // Touch states
    
    hal_touch_wheel_data_t wheel;                       // Touch wheel data
    hal_touch_button_data_t buttons[4];                 // Button data
    
    uint32_t timestamp;                                 // Data timestamp
    bool valid;                                         // Data validity flag
} hal_touch_data_t;

// Touch initialization and control
bool hal_touch_init(void);
void hal_touch_deinit(void);
bool hal_touch_is_initialized(void);

// Configuration management
bool hal_touch_set_sensitivity(hal_touch_sensitivity_t level);
hal_touch_sensitivity_t hal_touch_get_sensitivity(void);
bool hal_touch_set_electrode_config(uint8_t electrode, const hal_touch_electrode_config_t* config);
bool hal_touch_get_electrode_config(uint8_t electrode, hal_touch_electrode_config_t* config);

// Individual electrode configuration
bool hal_touch_set_electrode_thresholds(uint8_t electrode, uint8_t touch_thresh, uint8_t release_thresh);
bool hal_touch_enable_electrode(uint8_t electrode, bool enabled);
bool hal_touch_set_electrode_small_pad(uint8_t electrode, bool is_small);

// Data reading
bool hal_touch_read_data(hal_touch_data_t* data);
bool hal_touch_read_raw_data(uint16_t* raw_data);
bool hal_touch_read_filtered_data(uint16_t* filtered_data);
bool hal_touch_read_baseline(uint16_t* baseline);

// Touch state queries
bool hal_touch_is_electrode_touched(uint8_t electrode);
uint16_t hal_touch_get_touched_mask(void);              // Bitmask of touched electrodes
uint8_t hal_touch_get_touch_count(void);                // Number of touched electrodes

// Touch wheel operations
bool hal_touch_read_wheel(hal_touch_wheel_data_t* wheel_data);
float hal_touch_get_wheel_position(void);               // 0.0-1.0 position
float hal_touch_get_wheel_velocity(void);               // Rotation velocity
hal_touch_wheel_direction_t hal_touch_get_wheel_direction(void);

// Touch button operations
bool hal_touch_read_buttons(hal_touch_button_data_t* button_data);
hal_touch_state_t hal_touch_get_button_state(hal_touch_button_t button);
bool hal_touch_is_button_pressed(hal_touch_button_t button);
bool hal_touch_is_button_long_pressed(hal_touch_button_t button);

// Calibration and tuning
bool hal_touch_start_calibration(void);
bool hal_touch_stop_calibration(void);
bool hal_touch_is_calibrating(void);
bool hal_touch_reset_baseline(void);
bool hal_touch_auto_tune(uint32_t duration_ms);

// Advanced features
void hal_touch_set_debounce_time(uint32_t ms);
uint32_t hal_touch_get_debounce_time(void);
void hal_touch_set_long_press_time(uint32_t ms);
uint32_t hal_touch_get_long_press_time(void);

// Drift compensation
void hal_touch_enable_drift_compensation(bool enabled);
bool hal_touch_is_drift_compensation_enabled(void);
void hal_touch_set_drift_rate(uint8_t rate);           // Drift compensation rate

// Noise filtering
void hal_touch_set_filter_type(uint8_t filter_type);   // 0=none, 1=low-pass, 2=median
void hal_touch_set_filter_strength(uint8_t strength);  // Filter strength 0-255

// Interrupt and callback support
typedef void (*hal_touch_callback_t)(const hal_touch_data_t* data, void* user_data);
typedef void (*hal_touch_button_callback_t)(hal_touch_button_t button, hal_touch_state_t state, void* user_data);
typedef void (*hal_touch_wheel_callback_t)(const hal_touch_wheel_data_t* wheel, void* user_data);

void hal_touch_set_data_callback(hal_touch_callback_t callback, void* user_data);
void hal_touch_set_button_callback(hal_touch_button_callback_t callback, void* user_data);
void hal_touch_set_wheel_callback(hal_touch_wheel_callback_t callback, void* user_data);
void hal_touch_clear_callbacks(void);

// Configuration persistence
bool hal_touch_save_config(void);                      // Save to NVS
bool hal_touch_load_config(void);                      // Load from NVS
bool hal_touch_reset_config(void);                     // Reset to defaults

// Performance and debugging
void hal_touch_get_stats(uint32_t* reads, uint32_t* touches, uint32_t* errors);
void hal_touch_reset_stats(void);
bool hal_touch_self_test(void);                        // Hardware self-test

// Error handling
typedef enum {
    HAL_TOUCH_ERROR_NONE = 0,
    HAL_TOUCH_ERROR_INIT_FAILED = 1,
    HAL_TOUCH_ERROR_I2C_FAILED = 2,
    HAL_TOUCH_ERROR_INVALID_ELECTRODE = 3,
    HAL_TOUCH_ERROR_CALIBRATION_FAILED = 4,
    HAL_TOUCH_ERROR_CONFIG_INVALID = 5,
    HAL_TOUCH_ERROR_HARDWARE_FAULT = 6
} hal_touch_error_t;

hal_touch_error_t hal_touch_get_last_error(void);
const char* hal_touch_get_error_string(hal_touch_error_t error);

// Hardware-specific MPR121 functions
bool hal_touch_mpr121_read_register(uint8_t reg, uint8_t* value);
bool hal_touch_mpr121_write_register(uint8_t reg, uint8_t value);
bool hal_touch_mpr121_reset(void);
uint8_t hal_touch_mpr121_get_device_id(void);

#ifdef __cplusplus
}
#endif

