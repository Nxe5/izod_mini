/*
 * ESP32 Hardware Abstraction Layer - System Implementation
 * Uses Arduino/FreeRTOS APIs for system operations
 */

#include "hal/hal_system.h"
#include "hardware_config.h"

#ifdef PLATFORM_ESP32

#include <Arduino.h>
#include <esp_system.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <soc/rtc.h>
#include <driver/gpio.h>
#include <stdarg.h>

// Global system state
static bool g_initialized = false;
static hal_log_level_t g_log_level = HAL_LOG_LEVEL_INFO;
static hal_log_callback_t g_log_callback = nullptr;
static void* g_log_user_data = nullptr;
static hal_system_event_callback_t g_event_callback = nullptr;
static void* g_event_user_data = nullptr;

// System information cache
static hal_system_info_t g_system_info;
static bool g_system_info_cached = false;

// Task tracking
static uint32_t g_task_count = 0;

extern "C" {

// Internal helper functions
static void update_system_info(void);
static void log_output_handler(const char* format, va_list args);

// System initialization and control
bool hal_system_init(void) {
    if (g_initialized) {
        return true;
    }
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Update system information
    update_system_info();
    
    // Set up logging
    esp_log_level_set("*", ESP_LOG_INFO);
    
    g_initialized = true;
    
    // Send boot complete event
    if (g_event_callback) {
        g_event_callback(HAL_SYSTEM_EVENT_BOOT_COMPLETE, nullptr, g_event_user_data);
    }
    
    return true;
}

void hal_system_deinit(void) {
    if (!g_initialized) {
        return;
    }
    
    g_initialized = false;
}

bool hal_system_is_initialized(void) {
    return g_initialized;
}

// System information
bool hal_system_get_info(hal_system_info_t* info) {
    if (!info) return false;
    
    if (!g_system_info_cached) {
        update_system_info();
    }
    
    *info = g_system_info;
    return true;
}

const char* hal_system_get_chip_model(void) {
    if (!g_system_info_cached) {
        update_system_info();
    }
    return g_system_info.chip_model;
}

uint32_t hal_system_get_chip_revision(void) {
    if (!g_system_info_cached) {
        update_system_info();
    }
    return g_system_info.chip_revision;
}

uint32_t hal_system_get_cpu_frequency(void) {
    return rtc_clk_cpu_freq_get_config().freq_mhz;
}

const char* hal_system_get_firmware_version(void) {
    if (!g_system_info_cached) {
        update_system_info();
    }
    return g_system_info.firmware_version;
}

// Time and timing
uint32_t hal_system_get_time_ms(void) {
    return millis();
}

uint64_t hal_system_get_time_us(void) {
    return micros();
}

uint32_t hal_system_get_uptime_ms(void) {
    return millis();
}

void hal_system_delay_ms(uint32_t ms) {
    delay(ms);
}

void hal_system_delay_us(uint32_t us) {
    delayMicroseconds(us);
}

// Real-time clock
bool hal_system_set_rtc_time(uint32_t unix_timestamp) {
    // ESP32 doesn't have built-in RTC, would need external RTC module
    return false;
}

uint32_t hal_system_get_rtc_time(void) {
    // Return 0 if no RTC available
    return 0;
}

bool hal_system_is_rtc_valid(void) {
    return false;
}

// Memory management
uint32_t hal_system_get_free_heap(void) {
    return esp_get_free_heap_size();
}

uint32_t hal_system_get_total_heap(void) {
    return heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
}

uint32_t hal_system_get_min_free_heap(void) {
    return esp_get_minimum_free_heap_size();
}

uint32_t hal_system_get_max_alloc_heap(void) {
    return heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
}

void* hal_system_malloc(size_t size) {
    return malloc(size);
}

void* hal_system_calloc(size_t num, size_t size) {
    return calloc(num, size);
}

void* hal_system_realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}

void hal_system_free(void* ptr) {
    free(ptr);
}

// System reset and power control
void hal_system_reset(void) {
    esp_restart();
}

void hal_system_restart(void) {
    esp_restart();
}

hal_reset_reason_t hal_system_get_reset_reason(void) {
    esp_reset_reason_t reason = esp_reset_reason();
    
    switch (reason) {
        case ESP_RST_POWERON:   return HAL_RESET_REASON_POWER_ON;
        case ESP_RST_EXT:       return HAL_RESET_REASON_EXTERNAL;
        case ESP_RST_SW:        return HAL_RESET_REASON_SOFTWARE;
        case ESP_RST_WDT:       return HAL_RESET_REASON_WATCHDOG;
        case ESP_RST_DEEPSLEEP: return HAL_RESET_REASON_DEEP_SLEEP;
        case ESP_RST_BROWNOUT:  return HAL_RESET_REASON_BROWNOUT;
        case ESP_RST_PANIC:     return HAL_RESET_REASON_PANIC;
        default:                return HAL_RESET_REASON_UNKNOWN;
    }
}

// Power management
bool hal_system_set_power_mode(hal_power_mode_t mode) {
    switch (mode) {
        case HAL_POWER_MODE_ACTIVE:
            // Already in active mode
            return true;
            
        case HAL_POWER_MODE_LIGHT_SLEEP:
            esp_sleep_enable_timer_wakeup(1000000); // 1 second
            esp_light_sleep_start();
            return true;
            
        case HAL_POWER_MODE_DEEP_SLEEP:
            esp_deep_sleep_start();
            return true; // Won't actually return
            
        case HAL_POWER_MODE_HIBERNATE:
            esp_deep_sleep_start();
            return true; // Won't actually return
            
        default:
            return false;
    }
}

hal_power_mode_t hal_system_get_power_mode(void) {
    return HAL_POWER_MODE_ACTIVE; // Always active when running
}

bool hal_system_sleep(uint32_t duration_ms) {
    esp_sleep_enable_timer_wakeup(duration_ms * 1000);
    esp_light_sleep_start();
    return true;
}

bool hal_system_deep_sleep(uint32_t duration_ms) {
    esp_sleep_enable_timer_wakeup(duration_ms * 1000);
    esp_deep_sleep_start();
    return true; // Won't actually return
}

hal_wakeup_source_t hal_system_get_wakeup_source(void) {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER:    return HAL_WAKEUP_SOURCE_TIMER;
        case ESP_SLEEP_WAKEUP_GPIO:     return HAL_WAKEUP_SOURCE_GPIO;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: return HAL_WAKEUP_SOURCE_TOUCH;
        case ESP_SLEEP_WAKEUP_UART:     return HAL_WAKEUP_SOURCE_UART;
        case ESP_SLEEP_WAKEUP_EXT0:
        case ESP_SLEEP_WAKEUP_EXT1:     return HAL_WAKEUP_SOURCE_EXTERNAL;
        default:                        return HAL_WAKEUP_SOURCE_NONE;
    }
}

// CPU frequency scaling
bool hal_system_set_cpu_frequency(uint32_t freq_mhz) {
    rtc_cpu_freq_config_t config;
    
    if (freq_mhz == 240) {
        rtc_clk_cpu_freq_mhz_to_config(240, &config);
    } else if (freq_mhz == 160) {
        rtc_clk_cpu_freq_mhz_to_config(160, &config);
    } else if (freq_mhz == 80) {
        rtc_clk_cpu_freq_mhz_to_config(80, &config);
    } else {
        return false;
    }
    
    rtc_clk_cpu_freq_set_config(&config);
    return true;
}

uint32_t hal_system_get_cpu_frequency_current(void) {
    return rtc_clk_cpu_freq_get_config().freq_mhz;
}

uint32_t hal_system_get_cpu_frequency_max(void) {
    return 240; // ESP32 max frequency
}

// Watchdog timer
void hal_system_watchdog_enable(uint32_t timeout_ms) {
    esp_task_wdt_init(timeout_ms / 1000, true);
    esp_task_wdt_add(NULL);
}

void hal_system_watchdog_disable(void) {
    esp_task_wdt_delete(NULL);
    esp_task_wdt_deinit();
}

void hal_system_watchdog_feed(void) {
    esp_task_wdt_reset();
}

bool hal_system_is_watchdog_enabled(void) {
    // No direct way to check, assume enabled if initialized
    return g_initialized;
}

// Task management
hal_task_handle_t hal_system_create_task(hal_task_function_t function, 
                                         const char* name,
                                         uint32_t stack_size,
                                         void* parameters,
                                         hal_task_priority_t priority) {
    TaskHandle_t handle;
    UBaseType_t freertos_priority;
    
    // Convert HAL priority to FreeRTOS priority
    switch (priority) {
        case HAL_TASK_PRIORITY_IDLE:     freertos_priority = 0; break;
        case HAL_TASK_PRIORITY_LOW:      freertos_priority = 1; break;
        case HAL_TASK_PRIORITY_NORMAL:   freertos_priority = 2; break;
        case HAL_TASK_PRIORITY_HIGH:     freertos_priority = 3; break;
        case HAL_TASK_PRIORITY_CRITICAL: freertos_priority = 4; break;
        default:                         freertos_priority = 2; break;
    }
    
    BaseType_t result = xTaskCreate(
        (TaskFunction_t)function,
        name,
        stack_size / sizeof(StackType_t),
        parameters,
        freertos_priority,
        &handle
    );
    
    if (result == pdPASS) {
        g_task_count++;
        return (hal_task_handle_t)handle;
    }
    
    return nullptr;
}

void hal_system_delete_task(hal_task_handle_t task) {
    if (task) {
        vTaskDelete((TaskHandle_t)task);
        g_task_count--;
    }
}

void hal_system_suspend_task(hal_task_handle_t task) {
    if (task) {
        vTaskSuspend((TaskHandle_t)task);
    }
}

void hal_system_resume_task(hal_task_handle_t task) {
    if (task) {
        vTaskResume((TaskHandle_t)task);
    }
}

void hal_system_yield(void) {
    taskYIELD();
}

void hal_system_task_delay(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

// Task information
bool hal_system_get_task_info(hal_task_handle_t task, hal_task_info_t* info) {
    if (!task || !info) return false;
    
    TaskHandle_t handle = (TaskHandle_t)task;
    TaskStatus_t status;
    
    vTaskGetInfo(handle, &status, pdTRUE, eInvalid);
    
    strncpy(info->name, status.pcTaskName, HAL_SYSTEM_TASK_NAME_LENGTH - 1);
    info->name[HAL_SYSTEM_TASK_NAME_LENGTH - 1] = '\0';
    
    // Convert FreeRTOS priority to HAL priority
    if (status.uxCurrentPriority == 0) info->priority = HAL_TASK_PRIORITY_IDLE;
    else if (status.uxCurrentPriority == 1) info->priority = HAL_TASK_PRIORITY_LOW;
    else if (status.uxCurrentPriority == 2) info->priority = HAL_TASK_PRIORITY_NORMAL;
    else if (status.uxCurrentPriority == 3) info->priority = HAL_TASK_PRIORITY_HIGH;
    else info->priority = HAL_TASK_PRIORITY_CRITICAL;
    
    info->stack_size = status.usStackHighWaterMark * sizeof(StackType_t);
    info->stack_free = status.usStackHighWaterMark * sizeof(StackType_t);
    info->cpu_usage_percent = 0; // Not easily available
    info->running = (status.eCurrentState == eRunning);
    
    return true;
}

uint32_t hal_system_get_task_count(void) {
    return uxTaskGetNumberOfTasks();
}

bool hal_system_list_tasks(hal_task_info_t* tasks, uint32_t max_tasks, uint32_t* task_count) {
    if (!tasks || !task_count) return false;
    
    uint32_t num_tasks = uxTaskGetNumberOfTasks();
    if (num_tasks > max_tasks) num_tasks = max_tasks;
    
    TaskStatus_t* task_array = (TaskStatus_t*)malloc(num_tasks * sizeof(TaskStatus_t));
    if (!task_array) return false;
    
    uint32_t actual_count = uxTaskGetSystemState(task_array, num_tasks, nullptr);
    
    for (uint32_t i = 0; i < actual_count && i < max_tasks; i++) {
        strncpy(tasks[i].name, task_array[i].pcTaskName, HAL_SYSTEM_TASK_NAME_LENGTH - 1);
        tasks[i].name[HAL_SYSTEM_TASK_NAME_LENGTH - 1] = '\0';
        
        // Convert priority
        if (task_array[i].uxCurrentPriority == 0) tasks[i].priority = HAL_TASK_PRIORITY_IDLE;
        else if (task_array[i].uxCurrentPriority == 1) tasks[i].priority = HAL_TASK_PRIORITY_LOW;
        else if (task_array[i].uxCurrentPriority == 2) tasks[i].priority = HAL_TASK_PRIORITY_NORMAL;
        else if (task_array[i].uxCurrentPriority == 3) tasks[i].priority = HAL_TASK_PRIORITY_HIGH;
        else tasks[i].priority = HAL_TASK_PRIORITY_CRITICAL;
        
        tasks[i].stack_size = task_array[i].usStackHighWaterMark * sizeof(StackType_t);
        tasks[i].stack_free = task_array[i].usStackHighWaterMark * sizeof(StackType_t);
        tasks[i].cpu_usage_percent = 0;
        tasks[i].running = (task_array[i].eCurrentState == eRunning);
    }
    
    free(task_array);
    *task_count = actual_count;
    return true;
}

// Synchronization primitives
hal_mutex_t hal_system_create_mutex(void) {
    return (hal_mutex_t)xSemaphoreCreateMutex();
}

void hal_system_delete_mutex(hal_mutex_t mutex) {
    if (mutex) {
        vSemaphoreDelete((SemaphoreHandle_t)mutex);
    }
}

bool hal_system_take_mutex(hal_mutex_t mutex, uint32_t timeout_ms) {
    if (!mutex) return false;
    
    TickType_t ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake((SemaphoreHandle_t)mutex, ticks) == pdTRUE;
}

void hal_system_give_mutex(hal_mutex_t mutex) {
    if (mutex) {
        xSemaphoreGive((SemaphoreHandle_t)mutex);
    }
}

hal_semaphore_t hal_system_create_semaphore(uint32_t max_count, uint32_t initial_count) {
    return (hal_semaphore_t)xSemaphoreCreateCounting(max_count, initial_count);
}

void hal_system_delete_semaphore(hal_semaphore_t semaphore) {
    if (semaphore) {
        vSemaphoreDelete((SemaphoreHandle_t)semaphore);
    }
}

bool hal_system_take_semaphore(hal_semaphore_t semaphore, uint32_t timeout_ms) {
    if (!semaphore) return false;
    
    TickType_t ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake((SemaphoreHandle_t)semaphore, ticks) == pdTRUE;
}

void hal_system_give_semaphore(hal_semaphore_t semaphore) {
    if (semaphore) {
        xSemaphoreGive((SemaphoreHandle_t)semaphore);
    }
}

hal_queue_t hal_system_create_queue(uint32_t length, uint32_t item_size) {
    return (hal_queue_t)xQueueCreate(length, item_size);
}

void hal_system_delete_queue(hal_queue_t queue) {
    if (queue) {
        vQueueDelete((QueueHandle_t)queue);
    }
}

bool hal_system_queue_send(hal_queue_t queue, const void* item, uint32_t timeout_ms) {
    if (!queue) return false;
    
    TickType_t ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xQueueSend((QueueHandle_t)queue, item, ticks) == pdTRUE;
}

bool hal_system_queue_receive(hal_queue_t queue, void* item, uint32_t timeout_ms) {
    if (!queue) return false;
    
    TickType_t ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xQueueReceive((QueueHandle_t)queue, item, ticks) == pdTRUE;
}

// Logging system
void hal_system_set_log_level(hal_log_level_t level) {
    g_log_level = level;
    
    esp_log_level_t esp_level;
    switch (level) {
        case HAL_LOG_LEVEL_NONE:    esp_level = ESP_LOG_NONE; break;
        case HAL_LOG_LEVEL_ERROR:   esp_level = ESP_LOG_ERROR; break;
        case HAL_LOG_LEVEL_WARN:    esp_level = ESP_LOG_WARN; break;
        case HAL_LOG_LEVEL_INFO:    esp_level = ESP_LOG_INFO; break;
        case HAL_LOG_LEVEL_DEBUG:   esp_level = ESP_LOG_DEBUG; break;
        case HAL_LOG_LEVEL_VERBOSE: esp_level = ESP_LOG_VERBOSE; break;
        default:                    esp_level = ESP_LOG_INFO; break;
    }
    
    esp_log_level_set("*", esp_level);
}

hal_log_level_t hal_system_get_log_level(void) {
    return g_log_level;
}

void hal_system_log(hal_log_level_t level, const char* tag, const char* format, ...) {
    if (level > g_log_level) return;
    
    char buffer[HAL_SYSTEM_MAX_LOG_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Call custom callback if set
    if (g_log_callback) {
        g_log_callback(level, tag, buffer, g_log_user_data);
    }
    
    // Also output to ESP32 logging system
    esp_log_level_t esp_level;
    switch (level) {
        case HAL_LOG_LEVEL_ERROR:   esp_level = ESP_LOG_ERROR; break;
        case HAL_LOG_LEVEL_WARN:    esp_level = ESP_LOG_WARN; break;
        case HAL_LOG_LEVEL_INFO:    esp_level = ESP_LOG_INFO; break;
        case HAL_LOG_LEVEL_DEBUG:   esp_level = ESP_LOG_DEBUG; break;
        case HAL_LOG_LEVEL_VERBOSE: esp_level = ESP_LOG_VERBOSE; break;
        default:                    esp_level = ESP_LOG_INFO; break;
    }
    
    esp_log_write(esp_level, tag, "%s\n", buffer);
}

void hal_system_set_log_callback(hal_log_callback_t callback, void* user_data) {
    g_log_callback = callback;
    g_log_user_data = user_data;
}

// Random number generation
uint32_t hal_system_random(void) {
    return esp_random();
}

uint32_t hal_system_random_range(uint32_t min, uint32_t max) {
    if (min >= max) return min;
    return min + (esp_random() % (max - min));
}

void hal_system_random_bytes(uint8_t* buffer, size_t length) {
    esp_fill_random(buffer, length);
}

void hal_system_random_seed(uint32_t seed) {
    // ESP32 uses hardware RNG, seeding not needed
}

// Non-volatile storage (NVS) abstraction
bool hal_system_nvs_set_blob(const char* namespace_name, const char* key, const void* value, size_t length) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;
    
    err = nvs_set_blob(handle, key, value, length);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    
    nvs_close(handle);
    return (err == ESP_OK);
}

bool hal_system_nvs_get_blob(const char* namespace_name, const char* key, void* value, size_t* length) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READONLY, &handle);
    if (err != ESP_OK) return false;
    
    err = nvs_get_blob(handle, key, value, length);
    nvs_close(handle);
    
    return (err == ESP_OK);
}

bool hal_system_nvs_set_string(const char* namespace_name, const char* key, const char* value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;
    
    err = nvs_set_str(handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    
    nvs_close(handle);
    return (err == ESP_OK);
}

bool hal_system_nvs_get_string(const char* namespace_name, const char* key, char* value, size_t* length) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READONLY, &handle);
    if (err != ESP_OK) return false;
    
    err = nvs_get_str(handle, key, value, length);
    nvs_close(handle);
    
    return (err == ESP_OK);
}

bool hal_system_nvs_set_uint32(const char* namespace_name, const char* key, uint32_t value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;
    
    err = nvs_set_u32(handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    
    nvs_close(handle);
    return (err == ESP_OK);
}

bool hal_system_nvs_get_uint32(const char* namespace_name, const char* key, uint32_t* value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READONLY, &handle);
    if (err != ESP_OK) return false;
    
    err = nvs_get_u32(handle, key, value);
    nvs_close(handle);
    
    return (err == ESP_OK);
}

bool hal_system_nvs_erase_key(const char* namespace_name, const char* key) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;
    
    err = nvs_erase_key(handle, key);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    
    nvs_close(handle);
    return (err == ESP_OK);
}

bool hal_system_nvs_erase_namespace(const char* namespace_name) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;
    
    err = nvs_erase_all(handle);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    
    nvs_close(handle);
    return (err == ESP_OK);
}

// System events and callbacks
void hal_system_set_event_callback(hal_system_event_callback_t callback, void* user_data) {
    g_event_callback = callback;
    g_event_user_data = user_data;
}

// Performance monitoring
void hal_system_get_cpu_usage(uint32_t* usage_percent) {
    // CPU usage calculation would require task runtime stats
    if (usage_percent) *usage_percent = 50; // Placeholder
}

void hal_system_get_memory_usage(uint32_t* used_bytes, uint32_t* total_bytes) {
    if (total_bytes) *total_bytes = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    if (used_bytes) *used_bytes = *total_bytes - esp_get_free_heap_size();
}

void hal_system_get_task_stats(uint32_t* context_switches, uint32_t* interrupts) {
    // These stats would require FreeRTOS runtime stats
    if (context_switches) *context_switches = 0;
    if (interrupts) *interrupts = 0;
}

// Hardware-specific functions (ESP32)
bool hal_system_esp32_get_mac_address(uint8_t* mac) {
    if (!mac) return false;
    return esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK;
}

uint32_t hal_system_esp32_get_chip_id(void) {
    uint64_t mac = 0;
    esp_read_mac((uint8_t*)&mac, ESP_MAC_WIFI_STA);
    return (uint32_t)(mac >> 24);
}

float hal_system_esp32_get_temperature(void) {
    // ESP32 doesn't have built-in temperature sensor
    return 25.0f; // Placeholder
}

uint32_t hal_system_esp32_get_hall_sensor(void) {
    return hall_sensor_read();
}

// Error handling
hal_system_error_t hal_system_get_last_error(void) {
    return HAL_SYSTEM_ERROR_NONE; // Placeholder
}

const char* hal_system_get_error_string(hal_system_error_t error) {
    switch (error) {
        case HAL_SYSTEM_ERROR_NONE:                return "No error";
        case HAL_SYSTEM_ERROR_INIT_FAILED:         return "Initialization failed";
        case HAL_SYSTEM_ERROR_OUT_OF_MEMORY:       return "Out of memory";
        case HAL_SYSTEM_ERROR_INVALID_PARAMETER:   return "Invalid parameter";
        case HAL_SYSTEM_ERROR_TASK_CREATE_FAILED:  return "Task creation failed";
        case HAL_SYSTEM_ERROR_NVS_FAILED:          return "NVS operation failed";
        case HAL_SYSTEM_ERROR_HARDWARE_FAULT:      return "Hardware fault";
        default:                                   return "Unknown error";
    }
}

// Self-test and diagnostics
bool hal_system_self_test(void) {
    // Basic system health checks
    if (!g_initialized) return false;
    if (esp_get_free_heap_size() < 10000) return false; // Less than 10KB free
    
    return true;
}

void hal_system_dump_info(void) {
    if (!g_system_info_cached) {
        update_system_info();
    }
    
    hal_system_log(HAL_LOG_LEVEL_INFO, "SYSTEM", "=== System Information ===");
    hal_system_log(HAL_LOG_LEVEL_INFO, "SYSTEM", "Chip: %s rev %lu", 
                   g_system_info.chip_model, g_system_info.chip_revision);
    hal_system_log(HAL_LOG_LEVEL_INFO, "SYSTEM", "CPU: %lu MHz", g_system_info.cpu_freq_mhz);
    hal_system_log(HAL_LOG_LEVEL_INFO, "SYSTEM", "Flash: %lu MB", g_system_info.flash_size_mb);
    hal_system_log(HAL_LOG_LEVEL_INFO, "SYSTEM", "Heap: %lu/%lu bytes (min: %lu)", 
                   g_system_info.free_heap, g_system_info.total_heap, g_system_info.min_free_heap);
    hal_system_log(HAL_LOG_LEVEL_INFO, "SYSTEM", "Uptime: %lu ms", g_system_info.uptime_ms);
    hal_system_log(HAL_LOG_LEVEL_INFO, "SYSTEM", "Firmware: %s", g_system_info.firmware_version);
}

// Internal helper functions
static void update_system_info(void) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    // Chip model
    snprintf(g_system_info.chip_model, sizeof(g_system_info.chip_model), 
             "ESP32-PICO-V3-02");
    
    g_system_info.chip_revision = chip_info.revision;
    g_system_info.cpu_freq_mhz = rtc_clk_cpu_freq_get_config().freq_mhz;
    
    // Flash size
    uint32_t flash_size;
    esp_flash_get_size(NULL, &flash_size);
    g_system_info.flash_size_mb = flash_size / (1024 * 1024);
    
    // PSRAM size (ESP32-PICO-V3-02 doesn't have PSRAM)
    g_system_info.psram_size_mb = 0;
    
    // Memory info
    g_system_info.free_heap = esp_get_free_heap_size();
    g_system_info.total_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    g_system_info.min_free_heap = esp_get_minimum_free_heap_size();
    g_system_info.uptime_ms = millis();
    g_system_info.reset_reason = hal_system_get_reset_reason();
    
    // Firmware version
    #ifdef IZOD_FW_VERSION
    strncpy(g_system_info.firmware_version, IZOD_FW_VERSION, sizeof(g_system_info.firmware_version) - 1);
    #else
    strncpy(g_system_info.firmware_version, "1.0.0", sizeof(g_system_info.firmware_version) - 1);
    #endif
    
    // Build info
    strncpy(g_system_info.build_date, __DATE__, sizeof(g_system_info.build_date) - 1);
    strncpy(g_system_info.build_time, __TIME__, sizeof(g_system_info.build_time) - 1);
    
    g_system_info_cached = true;
}

} // extern "C"

#endif // PLATFORM_ESP32

