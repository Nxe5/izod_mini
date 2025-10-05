/*
 * Hardware Abstraction Layer - System Interface
 * Abstracts system operations for both ESP32 hardware and host emulation
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// System configuration constants
#define HAL_SYSTEM_MAX_LOG_LENGTH   256
#define HAL_SYSTEM_MAX_TASKS        16
#define HAL_SYSTEM_TASK_NAME_LENGTH 16

// Log levels
typedef enum {
    HAL_LOG_LEVEL_NONE    = 0,
    HAL_LOG_LEVEL_ERROR   = 1,
    HAL_LOG_LEVEL_WARN    = 2,
    HAL_LOG_LEVEL_INFO    = 3,
    HAL_LOG_LEVEL_DEBUG   = 4,
    HAL_LOG_LEVEL_VERBOSE = 5
} hal_log_level_t;

// System reset reasons
typedef enum {
    HAL_RESET_REASON_UNKNOWN     = 0,
    HAL_RESET_REASON_POWER_ON    = 1,
    HAL_RESET_REASON_EXTERNAL    = 2,
    HAL_RESET_REASON_SOFTWARE    = 3,
    HAL_RESET_REASON_WATCHDOG    = 4,
    HAL_RESET_REASON_DEEP_SLEEP  = 5,
    HAL_RESET_REASON_BROWNOUT    = 6,
    HAL_RESET_REASON_PANIC       = 7
} hal_reset_reason_t;

// Power modes
typedef enum {
    HAL_POWER_MODE_ACTIVE     = 0,
    HAL_POWER_MODE_LIGHT_SLEEP = 1,
    HAL_POWER_MODE_DEEP_SLEEP  = 2,
    HAL_POWER_MODE_HIBERNATE   = 3
} hal_power_mode_t;

// Wake-up sources
typedef enum {
    HAL_WAKEUP_SOURCE_NONE     = 0,
    HAL_WAKEUP_SOURCE_TIMER    = 1,
    HAL_WAKEUP_SOURCE_GPIO     = 2,
    HAL_WAKEUP_SOURCE_TOUCH    = 3,
    HAL_WAKEUP_SOURCE_UART     = 4,
    HAL_WAKEUP_SOURCE_EXTERNAL = 5
} hal_wakeup_source_t;

// Task priorities
typedef enum {
    HAL_TASK_PRIORITY_IDLE     = 0,
    HAL_TASK_PRIORITY_LOW      = 1,
    HAL_TASK_PRIORITY_NORMAL   = 2,
    HAL_TASK_PRIORITY_HIGH     = 3,
    HAL_TASK_PRIORITY_CRITICAL = 4
} hal_task_priority_t;

// System information structure
typedef struct {
    char chip_model[32];        // Chip model (ESP32-PICO-V3-02, etc.)
    uint32_t chip_revision;     // Chip revision
    uint32_t cpu_freq_mhz;      // CPU frequency in MHz
    uint32_t flash_size_mb;     // Flash size in MB
    uint32_t psram_size_mb;     // PSRAM size in MB
    uint32_t free_heap;         // Free heap memory in bytes
    uint32_t total_heap;        // Total heap memory in bytes
    uint32_t min_free_heap;     // Minimum free heap since boot
    uint32_t uptime_ms;         // System uptime in milliseconds
    hal_reset_reason_t reset_reason;
    char firmware_version[32];  // Firmware version string
    char build_date[32];        // Build date
    char build_time[32];        // Build time
} hal_system_info_t;

// Task information structure
typedef struct {
    char name[HAL_SYSTEM_TASK_NAME_LENGTH];
    hal_task_priority_t priority;
    uint32_t stack_size;
    uint32_t stack_free;
    uint32_t cpu_usage_percent;
    bool running;
} hal_task_info_t;

// Task handle type
typedef void* hal_task_handle_t;

// Task function type
typedef void (*hal_task_function_t)(void* parameters);

// Log callback type
typedef void (*hal_log_callback_t)(hal_log_level_t level, const char* tag, const char* message, void* user_data);

// System initialization and control
bool hal_system_init(void);
void hal_system_deinit(void);
bool hal_system_is_initialized(void);

// System information
bool hal_system_get_info(hal_system_info_t* info);
const char* hal_system_get_chip_model(void);
uint32_t hal_system_get_chip_revision(void);
uint32_t hal_system_get_cpu_frequency(void);
const char* hal_system_get_firmware_version(void);

// Time and timing
uint32_t hal_system_get_time_ms(void);          // Milliseconds since boot
uint64_t hal_system_get_time_us(void);          // Microseconds since boot
uint32_t hal_system_get_uptime_ms(void);        // System uptime
void hal_system_delay_ms(uint32_t ms);          // Blocking delay
void hal_system_delay_us(uint32_t us);          // Microsecond delay

// Real-time clock (if available)
bool hal_system_set_rtc_time(uint32_t unix_timestamp);
uint32_t hal_system_get_rtc_time(void);
bool hal_system_is_rtc_valid(void);

// Memory management
uint32_t hal_system_get_free_heap(void);
uint32_t hal_system_get_total_heap(void);
uint32_t hal_system_get_min_free_heap(void);
uint32_t hal_system_get_max_alloc_heap(void);
void* hal_system_malloc(size_t size);
void* hal_system_calloc(size_t num, size_t size);
void* hal_system_realloc(void* ptr, size_t size);
void hal_system_free(void* ptr);

// System reset and power control
void hal_system_reset(void);
void hal_system_restart(void);
hal_reset_reason_t hal_system_get_reset_reason(void);

// Power management
bool hal_system_set_power_mode(hal_power_mode_t mode);
hal_power_mode_t hal_system_get_power_mode(void);
bool hal_system_sleep(uint32_t duration_ms);
bool hal_system_deep_sleep(uint32_t duration_ms);
hal_wakeup_source_t hal_system_get_wakeup_source(void);

// CPU frequency scaling
bool hal_system_set_cpu_frequency(uint32_t freq_mhz);
uint32_t hal_system_get_cpu_frequency_current(void);
uint32_t hal_system_get_cpu_frequency_max(void);

// Watchdog timer
void hal_system_watchdog_enable(uint32_t timeout_ms);
void hal_system_watchdog_disable(void);
void hal_system_watchdog_feed(void);
bool hal_system_is_watchdog_enabled(void);

// Task management (FreeRTOS abstraction)
hal_task_handle_t hal_system_create_task(hal_task_function_t function, 
                                         const char* name,
                                         uint32_t stack_size,
                                         void* parameters,
                                         hal_task_priority_t priority);
void hal_system_delete_task(hal_task_handle_t task);
void hal_system_suspend_task(hal_task_handle_t task);
void hal_system_resume_task(hal_task_handle_t task);
void hal_system_yield(void);
void hal_system_task_delay(uint32_t ms);

// Task information
bool hal_system_get_task_info(hal_task_handle_t task, hal_task_info_t* info);
uint32_t hal_system_get_task_count(void);
bool hal_system_list_tasks(hal_task_info_t* tasks, uint32_t max_tasks, uint32_t* task_count);

// Synchronization primitives
typedef void* hal_mutex_t;
typedef void* hal_semaphore_t;
typedef void* hal_queue_t;

hal_mutex_t hal_system_create_mutex(void);
void hal_system_delete_mutex(hal_mutex_t mutex);
bool hal_system_take_mutex(hal_mutex_t mutex, uint32_t timeout_ms);
void hal_system_give_mutex(hal_mutex_t mutex);

hal_semaphore_t hal_system_create_semaphore(uint32_t max_count, uint32_t initial_count);
void hal_system_delete_semaphore(hal_semaphore_t semaphore);
bool hal_system_take_semaphore(hal_semaphore_t semaphore, uint32_t timeout_ms);
void hal_system_give_semaphore(hal_semaphore_t semaphore);

hal_queue_t hal_system_create_queue(uint32_t length, uint32_t item_size);
void hal_system_delete_queue(hal_queue_t queue);
bool hal_system_queue_send(hal_queue_t queue, const void* item, uint32_t timeout_ms);
bool hal_system_queue_receive(hal_queue_t queue, void* item, uint32_t timeout_ms);

// Logging system
void hal_system_set_log_level(hal_log_level_t level);
hal_log_level_t hal_system_get_log_level(void);
void hal_system_log(hal_log_level_t level, const char* tag, const char* format, ...);
void hal_system_set_log_callback(hal_log_callback_t callback, void* user_data);

// Convenience logging macros (when used from C++)
#define HAL_LOGE(tag, format, ...) hal_system_log(HAL_LOG_LEVEL_ERROR, tag, format, ##__VA_ARGS__)
#define HAL_LOGW(tag, format, ...) hal_system_log(HAL_LOG_LEVEL_WARN, tag, format, ##__VA_ARGS__)
#define HAL_LOGI(tag, format, ...) hal_system_log(HAL_LOG_LEVEL_INFO, tag, format, ##__VA_ARGS__)
#define HAL_LOGD(tag, format, ...) hal_system_log(HAL_LOG_LEVEL_DEBUG, tag, format, ##__VA_ARGS__)
#define HAL_LOGV(tag, format, ...) hal_system_log(HAL_LOG_LEVEL_VERBOSE, tag, format, ##__VA_ARGS__)

// Random number generation
uint32_t hal_system_random(void);
uint32_t hal_system_random_range(uint32_t min, uint32_t max);
void hal_system_random_bytes(uint8_t* buffer, size_t length);
void hal_system_random_seed(uint32_t seed);

// Non-volatile storage (NVS) abstraction
bool hal_system_nvs_set_blob(const char* namespace_name, const char* key, const void* value, size_t length);
bool hal_system_nvs_get_blob(const char* namespace_name, const char* key, void* value, size_t* length);
bool hal_system_nvs_set_string(const char* namespace_name, const char* key, const char* value);
bool hal_system_nvs_get_string(const char* namespace_name, const char* key, char* value, size_t* length);
bool hal_system_nvs_set_uint32(const char* namespace_name, const char* key, uint32_t value);
bool hal_system_nvs_get_uint32(const char* namespace_name, const char* key, uint32_t* value);
bool hal_system_nvs_erase_key(const char* namespace_name, const char* key);
bool hal_system_nvs_erase_namespace(const char* namespace_name);

// System events and callbacks
typedef enum {
    HAL_SYSTEM_EVENT_BOOT_COMPLETE = 0,
    HAL_SYSTEM_EVENT_LOW_MEMORY = 1,
    HAL_SYSTEM_EVENT_POWER_CHANGE = 2,
    HAL_SYSTEM_EVENT_WATCHDOG_WARNING = 3,
    HAL_SYSTEM_EVENT_TASK_OVERFLOW = 4
} hal_system_event_t;

typedef void (*hal_system_event_callback_t)(hal_system_event_t event, void* data, void* user_data);
void hal_system_set_event_callback(hal_system_event_callback_t callback, void* user_data);

// Performance monitoring
void hal_system_get_cpu_usage(uint32_t* usage_percent);
void hal_system_get_memory_usage(uint32_t* used_bytes, uint32_t* total_bytes);
void hal_system_get_task_stats(uint32_t* context_switches, uint32_t* interrupts);

// Hardware-specific functions (ESP32)
bool hal_system_esp32_get_mac_address(uint8_t* mac);
uint32_t hal_system_esp32_get_chip_id(void);
float hal_system_esp32_get_temperature(void);
uint32_t hal_system_esp32_get_hall_sensor(void);

// Error handling
typedef enum {
    HAL_SYSTEM_ERROR_NONE = 0,
    HAL_SYSTEM_ERROR_INIT_FAILED = 1,
    HAL_SYSTEM_ERROR_OUT_OF_MEMORY = 2,
    HAL_SYSTEM_ERROR_INVALID_PARAMETER = 3,
    HAL_SYSTEM_ERROR_TASK_CREATE_FAILED = 4,
    HAL_SYSTEM_ERROR_NVS_FAILED = 5,
    HAL_SYSTEM_ERROR_HARDWARE_FAULT = 6
} hal_system_error_t;

hal_system_error_t hal_system_get_last_error(void);
const char* hal_system_get_error_string(hal_system_error_t error);

// Self-test and diagnostics
bool hal_system_self_test(void);
void hal_system_dump_info(void);

#ifdef __cplusplus
}
#endif

