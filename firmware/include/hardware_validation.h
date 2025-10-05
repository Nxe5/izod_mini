/*
 * Hardware Validation Header
 * Provides hardware testing and validation functions
 */

#ifndef HARDWARE_VALIDATION_H
#define HARDWARE_VALIDATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate hardware configuration at runtime
 * @return true if hardware configuration is valid, false otherwise
 */
bool hardware_config_validate(void);

/**
 * @brief Initialize hardware pins and peripherals
 * @return true if initialization successful, false otherwise
 */
bool hardware_config_init(void);

/**
 * @brief Get hardware version string
 * @return Hardware version string
 */
const char* hardware_get_version_string(void);

/**
 * @brief Check if a specific hardware feature is available
 * @param feature Feature flag to check
 * @return true if feature is available, false otherwise
 */
bool hardware_feature_available(uint32_t feature);

/**
 * @brief Print hardware diagnostics information
 */
void hardware_print_diagnostics(void);

/**
 * @brief Run comprehensive hardware self-test
 * @return true if all tests pass, false otherwise
 */
bool hardware_run_self_test(void);

#ifdef __cplusplus
}
#endif

#endif // HARDWARE_VALIDATION_H
