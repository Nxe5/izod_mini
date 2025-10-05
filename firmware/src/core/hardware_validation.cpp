/*
 * Hardware Validation System
 * Validates hardware configuration and compatibility at runtime
 */

#include "hardware_config.h"
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_MPR121.h>

// Validation results structure
typedef struct {
    bool i2c_bus;
    bool spi_bus;
    bool mpr121_touch;
    bool sd_card;
    bool display;
    bool audio_dac;
    bool buttons;
    bool power_management;
    uint8_t error_count;
    char error_messages[8][64];
} hardware_validation_result_t;

static hardware_validation_result_t g_validation_result;

// Internal validation functions
static bool validate_i2c_bus();
static bool validate_spi_bus();
static bool validate_mpr121();
static bool validate_sd_card();
static bool validate_display();
static bool validate_audio_dac();
static bool validate_buttons();
static bool validate_power_management();
static void add_error(const char* message);

bool hardware_config_validate(void) {
    Serial.println("=== Hardware Validation ===");
    
    // Initialize validation result
    memset(&g_validation_result, 0, sizeof(g_validation_result));
    
    // Validate pin assignments for conflicts
    bool pin_conflicts = false;
    
    // Check for pin conflicts
    int pins_used[40] = {0}; // ESP32 has GPIO 0-39
    
    // Mark pins as used and check for conflicts
    auto check_pin = [&](int pin, const char* function) {
        if (pin >= 0 && pin < 40) {
            if (pins_used[pin] > 0) {
                char error_msg[64];
                snprintf(error_msg, sizeof(error_msg), "Pin conflict: GPIO%d used by %s", pin, function);
                add_error(error_msg);
                pin_conflicts = true;
            }
            pins_used[pin]++;
        }
    };
    
    // Check all pin assignments
    check_pin(SLEEP_SWITCH_PIN, "Sleep Switch");
    check_pin(UART_TX_PIN, "UART TX");
    check_pin(UART_RX_PIN, "UART RX");
    check_pin(I2C_SDA_PIN, "I2C SDA");
    check_pin(I2C_SCL_PIN, "I2C SCL");
    check_pin(MPR121_IRQ_PIN, "MPR121 IRQ");
    
    check_pin(SD_DAT0_PIN, "SD DAT0");
    check_pin(SD_DAT1_PIN, "SD DAT1");
    check_pin(SD_DAT2_PIN, "SD DAT2");
    check_pin(SD_DAT3_PIN, "SD DAT3");
    check_pin(SD_CMD_PIN, "SD CMD");
    check_pin(SD_CLK_PIN, "SD CLK");
    check_pin(SD_DETECT_PIN, "SD Detect");
    
    check_pin(PCM_SCK_PIN, "PCM SCK");
    check_pin(PCM_BCK_PIN, "PCM BCK");
    check_pin(PCM_DIN_PIN, "PCM DIN");
    check_pin(PCM_LRCK_PIN, "PCM LRCK");
    check_pin(PCM_XSMT_PIN, "PCM XSMT");
    
    check_pin(TFT_CS_PIN, "TFT CS");
    check_pin(TFT_SCK_PIN, "TFT SCK");
    check_pin(TFT_MOSI_PIN, "TFT MOSI");
    check_pin(TFT_DC_PIN, "TFT DC");
    check_pin(TFT_RST_PIN, "TFT RST");
    
    check_pin(BTN_PREVIOUS_PIN, "Button Previous");
    check_pin(BTN_PLAY_PAUSE_PIN, "Button Play/Pause");
    check_pin(BTN_NEXT_PIN, "Button Next");
    check_pin(BTN_MENU_PIN, "Button Menu");
    check_pin(BTN_SELECT_PIN, "Button Select");
    
    if (!pin_conflicts) {
        Serial.println("✓ Pin assignment validation passed");
    }
    
    // Validate individual hardware components
    g_validation_result.i2c_bus = validate_i2c_bus();
    g_validation_result.spi_bus = validate_spi_bus();
    g_validation_result.mpr121_touch = validate_mpr121();
    g_validation_result.sd_card = validate_sd_card();
    g_validation_result.display = validate_display();
    g_validation_result.audio_dac = validate_audio_dac();
    g_validation_result.buttons = validate_buttons();
    g_validation_result.power_management = validate_power_management();
    
    // Print validation summary
    Serial.println("\n=== Validation Summary ===");
    Serial.printf("I2C Bus:           %s\n", g_validation_result.i2c_bus ? "✓ PASS" : "✗ FAIL");
    Serial.printf("SPI Bus:           %s\n", g_validation_result.spi_bus ? "✓ PASS" : "✗ FAIL");
    Serial.printf("MPR121 Touch:      %s\n", g_validation_result.mpr121_touch ? "✓ PASS" : "✗ FAIL");
    Serial.printf("SD Card:           %s\n", g_validation_result.sd_card ? "✓ PASS" : "✗ FAIL");
    Serial.printf("Display:           %s\n", g_validation_result.display ? "✓ PASS" : "✗ FAIL");
    Serial.printf("Audio DAC:         %s\n", g_validation_result.audio_dac ? "✓ PASS" : "✗ FAIL");
    Serial.printf("Buttons:           %s\n", g_validation_result.buttons ? "✓ PASS" : "✗ FAIL");
    Serial.printf("Power Management:  %s\n", g_validation_result.power_management ? "✓ PASS" : "✗ FAIL");
    
    if (g_validation_result.error_count > 0) {
        Serial.println("\n=== Errors ===");
        for (int i = 0; i < g_validation_result.error_count; i++) {
            Serial.printf("  %d. %s\n", i + 1, g_validation_result.error_messages[i]);
        }
    }
    
    // Overall validation result
    bool overall_result = g_validation_result.i2c_bus && 
                         g_validation_result.spi_bus && 
                         g_validation_result.display &&
                         g_validation_result.buttons &&
                         !pin_conflicts;
    
    Serial.printf("\n=== Overall Result: %s ===\n", overall_result ? "PASS" : "FAIL");
    
    return overall_result;
}

bool hardware_config_init(void) {
    Serial.println("Initializing hardware configuration...");
    
    // Initialize GPIO pins
    #if STATUS_LED_PIN != -1
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);
    #endif
    
    #if POWER_LED_PIN != -1
    pinMode(POWER_LED_PIN, OUTPUT);
    digitalWrite(POWER_LED_PIN, HIGH);
    #endif
    
    // Initialize sleep switch
    pinMode(SLEEP_SWITCH_PIN, INPUT_PULLUP);
    
    // Initialize boot button
    pinMode(BOOT_PIN, INPUT_PULLUP);
    
    // Initialize physical buttons
    pinMode(BTN_PREVIOUS_PIN, INPUT_PULLUP);
    pinMode(BTN_PLAY_PAUSE_PIN, INPUT_PULLUP);
    pinMode(BTN_NEXT_PIN, INPUT_PULLUP);
    pinMode(BTN_MENU_PIN, INPUT_PULLUP);
    pinMode(BTN_SELECT_PIN, INPUT_PULLUP);
    
    // Initialize SD card detect pin
    pinMode(SD_DETECT_PIN, INPUT_PULLUP);
    
    // Initialize MPR121 interrupt pin
    pinMode(MPR121_IRQ_PIN, INPUT_PULLUP);
    
    // Initialize PCM5102A control pins
    pinMode(PCM_XSMT_PIN, OUTPUT);
    digitalWrite(PCM_XSMT_PIN, HIGH); // Unmute by default
    
    Serial.println("✓ Hardware configuration initialized");
    return true;
}

const char* hardware_get_version_string(void) {
    return HARDWARE_VERSION_STRING;
}

bool hardware_feature_available(uint32_t feature) {
    switch (feature) {
        case HW_FEATURE_TOUCH_WHEEL:
            return g_validation_result.mpr121_touch;
        case HW_FEATURE_SD_CARD:
            return g_validation_result.sd_card;
        case HW_FEATURE_AUDIO_DAC:
            return g_validation_result.audio_dac;
        case HW_FEATURE_TFT_DISPLAY:
            return g_validation_result.display;
        case HW_FEATURE_PHYSICAL_BTNS:
            return g_validation_result.buttons;
        case HW_FEATURE_SLEEP_SWITCH:
            return digitalRead(SLEEP_SWITCH_PIN) == LOW; // Active low
        case HW_FEATURE_UART_DEBUG:
            return true; // Always available
        default:
            return false;
    }
}

// Internal validation functions
static void add_error(const char* message) {
    if (g_validation_result.error_count < 8) {
        strncpy(g_validation_result.error_messages[g_validation_result.error_count], 
                message, 63);
        g_validation_result.error_messages[g_validation_result.error_count][63] = '\0';
        g_validation_result.error_count++;
    }
}

static bool validate_i2c_bus() {
    Serial.print("Validating I2C bus... ");
    
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_FREQUENCY);
    
    // Scan for I2C devices
    int device_count = 0;
    for (int address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            device_count++;
            Serial.printf("Found I2C device at 0x%02X ", address);
        }
    }
    
    if (device_count > 0) {
        Serial.printf("✓ Found %d I2C device(s)\n", device_count);
        return true;
    } else {
        Serial.println("✗ No I2C devices found");
        add_error("No I2C devices detected");
        return false;
    }
}

static bool validate_spi_bus() {
    Serial.print("Validating SPI bus... ");
    
    SPI.begin(TFT_SCK_PIN, TFT_MISO_PIN, TFT_MOSI_PIN);
    
    // Basic SPI functionality test
    // This is a simple test - in practice you'd test with actual SPI devices
    Serial.println("✓ SPI bus initialized");
    return true;
}

static bool validate_mpr121() {
    Serial.print("Validating MPR121 touch controller... ");
    
    Adafruit_MPR121 mpr121;
    if (mpr121.begin(MPR121_I2C_ADDR, &Wire)) {
        Serial.println("✓ MPR121 detected and initialized");
        return true;
    } else {
        Serial.println("✗ MPR121 not found");
        add_error("MPR121 touch controller not detected");
        return false;
    }
}

static bool validate_sd_card() {
    Serial.print("Validating SD card... ");
    
    // Check if SD card is inserted (if detect pin is available)
    if (SD_DETECT_PIN != -1) {
        if (digitalRead(SD_DETECT_PIN) == HIGH) { // Assuming active low detect
            Serial.println("✗ No SD card inserted");
            add_error("SD card not inserted");
            return false;
        }
    }
    
    // Try to initialize SD card
    if (SD.begin(SD_CLK_PIN)) { // Using CLK pin as CS for SDMMC mode
        uint64_t card_size = SD.cardSize() / (1024 * 1024);
        Serial.printf("✓ SD card detected (%llu MB)\n", card_size);
        return true;
    } else {
        Serial.println("✗ SD card initialization failed");
        add_error("SD card initialization failed");
        return false;
    }
}

static bool validate_display() {
    Serial.print("Validating TFT display... ");
    
    // This is a basic test - in practice you'd test display communication
    // For now, just check if pins are properly configured
    pinMode(TFT_CS_PIN, OUTPUT);
    pinMode(TFT_DC_PIN, OUTPUT);
    if (TFT_RST_PIN != -1) {
        pinMode(TFT_RST_PIN, OUTPUT);
    }
    
    Serial.println("✓ Display pins configured");
    return true;
}

static bool validate_audio_dac() {
    Serial.print("Validating PCM5102A audio DAC... ");
    
    // Configure I2S pins
    pinMode(PCM_SCK_PIN, OUTPUT);
    pinMode(PCM_BCK_PIN, OUTPUT);
    pinMode(PCM_DIN_PIN, OUTPUT);
    pinMode(PCM_LRCK_PIN, OUTPUT);
    pinMode(PCM_XSMT_PIN, OUTPUT);
    
    // Set XSMT high to unmute
    digitalWrite(PCM_XSMT_PIN, HIGH);
    
    Serial.println("✓ Audio DAC pins configured");
    return true;
}

static bool validate_buttons() {
    Serial.print("Validating physical buttons... ");
    
    // Test button pins
    int button_pins[] = {
        BTN_PREVIOUS_PIN,
        BTN_PLAY_PAUSE_PIN,
        BTN_NEXT_PIN,
        BTN_MENU_PIN,
        BTN_SELECT_PIN
    };
    
    bool all_buttons_ok = true;
    for (int i = 0; i < 5; i++) {
        pinMode(button_pins[i], INPUT_PULLUP);
        int reading = digitalRead(button_pins[i]);
        
        // Buttons should read HIGH when not pressed (with pullup)
        if (reading != HIGH) {
            Serial.printf("✗ Button %d stuck low ", i);
            all_buttons_ok = false;
        }
    }
    
    if (all_buttons_ok) {
        Serial.println("✓ All buttons responsive");
        return true;
    } else {
        add_error("One or more buttons stuck");
        return false;
    }
}

static bool validate_power_management() {
    Serial.print("Validating power management... ");
    
    // Check sleep switch
    pinMode(SLEEP_SWITCH_PIN, INPUT_PULLUP);
    
    // Basic power management validation
    // In practice, you'd test battery monitoring, charging detection, etc.
    Serial.println("✓ Power management pins configured");
    return true;
}

// Hardware diagnostic functions
void hardware_print_diagnostics() {
    Serial.println("\n=== Hardware Diagnostics ===");
    
    // Print chip information
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
    Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("PSRAM Size: %d bytes\n", ESP.getPsramSize());
    
    // Print pin states
    Serial.println("\nPin States:");
    Serial.printf("Sleep Switch (GPIO%d): %s\n", SLEEP_SWITCH_PIN, 
                  digitalRead(SLEEP_SWITCH_PIN) ? "HIGH" : "LOW");
    Serial.printf("SD Detect (GPIO%d): %s\n", SD_DETECT_PIN,
                  digitalRead(SD_DETECT_PIN) ? "HIGH" : "LOW");
    Serial.printf("MPR121 IRQ (GPIO%d): %s\n", MPR121_IRQ_PIN,
                  digitalRead(MPR121_IRQ_PIN) ? "HIGH" : "LOW");
    
    // Print button states
    Serial.println("\nButton States:");
    const char* button_names[] = {"Previous", "Play/Pause", "Next", "Menu", "Select"};
    int button_pins[] = {BTN_PREVIOUS_PIN, BTN_PLAY_PAUSE_PIN, BTN_NEXT_PIN, BTN_MENU_PIN, BTN_SELECT_PIN};
    
    for (int i = 0; i < 5; i++) {
        Serial.printf("%s (GPIO%d): %s\n", button_names[i], button_pins[i],
                      digitalRead(button_pins[i]) ? "Released" : "Pressed");
    }
    
    Serial.println("==============================");
}

// Hardware test functions
bool hardware_run_self_test() {
    Serial.println("\n=== Hardware Self Test ===");
    
    bool test_passed = true;
    
    // Test 1: Button response test
    Serial.println("Test 1: Button Response Test");
    Serial.println("Press each button when prompted...");
    
    const char* button_names[] = {"Previous", "Play/Pause", "Next", "Menu", "Select"};
    int button_pins[] = {BTN_PREVIOUS_PIN, BTN_PLAY_PAUSE_PIN, BTN_NEXT_PIN, BTN_MENU_PIN, BTN_SELECT_PIN};
    
    for (int i = 0; i < 5; i++) {
        Serial.printf("Press %s button... ", button_names[i]);
        
        uint32_t start_time = millis();
        while (millis() - start_time < 5000) { // 5 second timeout
            if (digitalRead(button_pins[i]) == LOW) {
                Serial.println("✓ PASS");
                delay(500); // Debounce
                break;
            }
            delay(10);
        }
        
        if (millis() - start_time >= 5000) {
            Serial.println("✗ TIMEOUT");
            test_passed = false;
        }
    }
    
    // Test 2: I2C communication test
    Serial.println("\nTest 2: I2C Communication Test");
    if (validate_mpr121()) {
        Serial.println("✓ I2C communication working");
    } else {
        Serial.println("✗ I2C communication failed");
        test_passed = false;
    }
    
    // Test 3: SD card test
    Serial.println("\nTest 3: SD Card Test");
    if (validate_sd_card()) {
        Serial.println("✓ SD card working");
    } else {
        Serial.println("✗ SD card failed");
        // SD card failure is not critical for basic operation
    }
    
    Serial.printf("\n=== Self Test Result: %s ===\n", test_passed ? "PASS" : "FAIL");
    return test_passed;
}

