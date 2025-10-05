#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

/*
 * Izod Mini Hardware Configuration
 * ESP32-PICO-V3-02 Pin Definitions
 * 
 * This file centralizes all hardware pin assignments and configurations
 * for the Izod Mini music player project.
 */

#include <stdint.h>

// =============================================================================
// Hardware Version and Compatibility
// =============================================================================
#define HARDWARE_VERSION_MAJOR  1
#define HARDWARE_VERSION_MINOR  0
#define HARDWARE_VERSION_PATCH  0
#define HARDWARE_VERSION_STRING "1.0.0"

#define TARGET_CHIP_ESP32_PICO_V3_02
#define CHIP_FLASH_SIZE_MB      4
#define CHIP_PSRAM_SIZE_MB      0

// =============================================================================
// Power Management and System Control
// =============================================================================
#define SLEEP_SWITCH_PIN        21      // Sleep switch input
#define BOOT_PIN               0        // Boot button (GPIO0)
#define POWER_LED_PIN          -1       // No dedicated power LED
#define STATUS_LED_PIN         -1       // No dedicated status LED

// =============================================================================
// USB-to-UART Converter (CH9102F)
// =============================================================================
#define UART_TX_PIN            1        // UART TX to CH9102F
#define UART_RX_PIN            3        // UART RX from CH9102F (note: not pin 2 as specified)
#define UART_BAUD_RATE         115200   // Default UART baud rate

// =============================================================================
// I2C Bus Configuration (MPR121 Touch Controller)
// =============================================================================
#define I2C_SDA_PIN            22       // I2C SDA line
#define I2C_SCL_PIN            19       // I2C SCL line
#define I2C_FREQUENCY          400000   // I2C frequency (400kHz)

// MPR121 Touch Controller
#define MPR121_I2C_ADDR        0x5A     // MPR121 I2C address
#define MPR121_IRQ_PIN         18       // MPR121 interrupt pin
#define MPR121_ELECTRODE_COUNT 12       // Number of electrodes

// =============================================================================
// SD Card Interface (SDMMC 4-bit mode)
// =============================================================================
#define SD_DAT0_PIN            7        // SD Card DAT0
#define SD_DAT1_PIN            8        // SD Card DAT1  
#define SD_DAT2_PIN            9        // SD Card DAT2
#define SD_DAT3_PIN            10       // SD Card DAT3
#define SD_CMD_PIN             11       // SD Card CMD
#define SD_CLK_PIN             6        // SD Card CLK
#define SD_DETECT_PIN          20       // SD Card detect switch
#define SD_MAX_FREQUENCY       20000000 // SD Card max frequency (20MHz)

// =============================================================================
// Audio DAC (PCM5102A) - I2S Interface
// =============================================================================
#define PCM_SCK_PIN            12       // PCM5102A SCK (System Clock)
#define PCM_BCK_PIN            13       // PCM5102A BCK (Bit Clock)
#define PCM_DIN_PIN            15       // PCM5102A DIN (Data Input)
#define PCM_LRCK_PIN           2        // PCM5102A LRCK (Left/Right Clock)
#define PCM_XSMT_PIN           4        // PCM5102A XSMT (Soft Mute)

// I2S Configuration
#define I2S_SAMPLE_RATE        44100    // Default sample rate
#define I2S_BITS_PER_SAMPLE    16       // Bits per sample
#define I2S_CHANNELS           2        // Stereo channels
#define I2S_BUFFER_COUNT       8        // Number of DMA buffers
#define I2S_BUFFER_SIZE        1024     // Size of each DMA buffer

// =============================================================================
// TFT Display (ST7789 or similar) - SPI Interface
// =============================================================================
#define TFT_CS_PIN             33       // TFT Chip Select
#define TFT_SCK_PIN            25       // TFT SPI Clock
#define TFT_MOSI_PIN           26       // TFT SPI MOSI
#define TFT_MISO_PIN           -1       // TFT SPI MISO (not used)
#define TFT_DC_PIN             32       // TFT Data/Command
#define TFT_RST_PIN            27       // TFT Reset (was TFT_A0_PIN)
#define TFT_BL_PIN             -1       // TFT Backlight (not used)

// Display Configuration
#define TFT_WIDTH              240      // Display width in pixels
#define TFT_HEIGHT             320      // Display height in pixels
#define TFT_ROTATION           3        // Display rotation (landscape)
#define TFT_SPI_FREQUENCY      40000000 // SPI frequency (40MHz)

// =============================================================================
// Physical Control Buttons
// =============================================================================
#define BTN_PREVIOUS_PIN       34       // Previous track button
#define BTN_PLAY_PAUSE_PIN     39       // Play/Pause button
#define BTN_NEXT_PIN           38       // Next track button
#define BTN_MENU_PIN           37       // Menu button
#define BTN_SELECT_PIN         36       // Select/Center button

#define BTN_DEBOUNCE_MS        50       // Button debounce time
#define BTN_LONG_PRESS_MS      1000     // Long press threshold
#define BTN_REPEAT_MS          200      // Button repeat rate

// Button active state (true = active low, false = active high)
#define BTN_ACTIVE_LOW         true

// =============================================================================
// Pin Validation and Conflict Detection
// =============================================================================

// Compile-time pin conflict detection
#if (PCM_LRCK_PIN == UART_RX_PIN)
    #warning "PCM_LRCK_PIN conflicts with UART_RX_PIN - shared pin usage"
#endif

// Validate pin ranges for ESP32-PICO-V3-02
#define PIN_VALID(pin) ((pin >= 0 && pin <= 39) || pin == -1)

#if !PIN_VALID(SLEEP_SWITCH_PIN)
    #error "SLEEP_SWITCH_PIN out of valid range"
#endif

// =============================================================================
// Hardware Feature Flags
// =============================================================================
#define HW_FEATURE_TOUCH_WHEEL     1    // MPR121 touch wheel available
#define HW_FEATURE_SD_CARD         1    // SD card interface available
#define HW_FEATURE_AUDIO_DAC       1    // PCM5102A DAC available
#define HW_FEATURE_TFT_DISPLAY     1    // TFT display available
#define HW_FEATURE_PHYSICAL_BTNS   1    // Physical buttons available
#define HW_FEATURE_SLEEP_SWITCH    1    // Sleep switch available
#define HW_FEATURE_UART_DEBUG      1    // UART debug interface available

// =============================================================================
// Memory and Performance Configuration
// =============================================================================
#define HEAP_SIZE_KB           256      // Available heap size
#define STACK_SIZE_KB          8        // Default stack size
#define TASK_PRIORITY_HIGH     3        // High priority tasks
#define TASK_PRIORITY_NORMAL   2        // Normal priority tasks
#define TASK_PRIORITY_LOW      1        // Low priority tasks

// =============================================================================
// Power Management
// =============================================================================
#define POWER_SLEEP_TIMEOUT_MS     30000    // Auto-sleep timeout
#define POWER_DEEP_SLEEP_TIMEOUT_MS 300000  // Deep sleep timeout
#define POWER_LOW_BATTERY_MV       3300     // Low battery threshold
#define POWER_CRITICAL_BATTERY_MV  3000     // Critical battery threshold

// =============================================================================
// Hardware Validation Functions
// =============================================================================
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

#ifdef __cplusplus
}
#endif

#endif // HARDWARE_CONFIG_H

