# Hardware Abstraction Layer (HAL) Documentation

## Overview

The Izod Mini firmware uses a Hardware Abstraction Layer (HAL) to separate application logic from hardware-specific implementations. This allows the same code to run on both ESP32 hardware and host systems for development and testing.

## Architecture

```
Application Layer
       |
   HAL Interface
       |
+------+------+
|             |
ESP32 HAL    Host HAL
(Hardware)   (Emulation)
```

## HAL Modules

### 1. Display HAL (`hal_display.h`)
- **Purpose**: Abstract TFT display operations
- **ESP32 Implementation**: Uses Adafruit_ST7789 library
- **Host Implementation**: Uses SDL2 for window-based emulation
- **Features**: Drawing primitives, text rendering, color management

### 2. System HAL (`hal_system.h`)
- **Purpose**: Abstract system operations (time, memory, tasks, logging)
- **ESP32 Implementation**: Uses Arduino/FreeRTOS APIs
- **Host Implementation**: Uses standard C++ libraries
- **Features**: Task management, memory allocation, NVS storage, logging

### 3. Audio HAL (`hal_audio.h`)
- **Purpose**: Abstract PCM5102A audio operations
- **ESP32 Implementation**: Uses ESP8266Audio library (to be implemented)
- **Host Implementation**: Uses SDL2 audio (to be implemented)
- **Features**: Playback control, volume management, format support

### 4. Touch HAL (`hal_touch.h`)
- **Purpose**: Abstract MPR121 touch operations
- **ESP32 Implementation**: Uses Adafruit_MPR121 library (to be implemented)
- **Host Implementation**: Uses mouse/keyboard simulation (to be implemented)
- **Features**: Touch wheel, button detection, sensitivity control

### 5. Storage HAL (`hal_storage.h`)
- **Purpose**: Abstract SD card operations
- **ESP32 Implementation**: Uses SD library (to be implemented)
- **Host Implementation**: Uses filesystem operations (to be implemented)
- **Features**: File I/O, directory management, path utilities

## Build Targets

### ESP32 Hardware Targets

#### `esp32-hardware`
- **Platform**: ESP32-PICO-V3-02
- **Framework**: Arduino
- **HAL**: ESP32-specific implementations
- **Use Case**: Production firmware

#### `esp32-hardware-debug`
- **Extends**: `esp32-hardware`
- **Features**: Debug symbols, verbose logging, touch debugging
- **Use Case**: Hardware debugging

#### `esp32-hardware-release`
- **Extends**: `esp32-hardware`
- **Features**: Optimized build, minimal logging
- **Use Case**: Production release

### Host Emulation Targets

#### `native-emulation`
- **Platform**: Native (Linux/macOS/Windows)
- **Framework**: None (standard C++)
- **HAL**: SDL2-based implementations
- **Use Case**: Interactive development and testing

#### `native-test`
- **Platform**: Native
- **Framework**: Unity testing framework
- **HAL**: Mock implementations
- **Use Case**: Automated unit testing

## Usage Examples

### Basic HAL Usage

```cpp
#include "hal/hal_display.h"
#include "hal/hal_system.h"

int main() {
    // Initialize HAL
    if (!hal_system_init()) {
        return -1;
    }
    
    if (!hal_display_init()) {
        return -1;
    }
    
    // Use HAL functions
    hal_display_clear(HAL_COLOR_BLACK);
    hal_display_set_text_color(HAL_COLOR_WHITE);
    hal_display_printf("Hello, HAL!");
    hal_display_update();
    
    hal_system_delay_ms(1000);
    
    // Cleanup
    hal_display_deinit();
    hal_system_deinit();
    
    return 0;
}
```

### Platform-Specific Code

```cpp
#ifdef PLATFORM_ESP32
    // ESP32-specific initialization
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
#endif

#ifdef PLATFORM_HOST
    // Host-specific initialization
    printf("Running on host platform\n");
#endif

// Common HAL code works on both platforms
hal_display_clear(HAL_COLOR_BLUE);
hal_system_log(HAL_LOG_LEVEL_INFO, "APP", "Platform initialized");
```

## Building

### ESP32 Hardware Build

```bash
# Standard build
pio run -e esp32-hardware

# Debug build
pio run -e esp32-hardware-debug

# Release build
pio run -e esp32-hardware-release

# Upload to device
pio run -e esp32-hardware -t upload

# Monitor serial output
pio run -e esp32-hardware -t monitor
```

### Host Emulation Build

```bash
# Build emulation
pio run -e native-emulation

# Run emulation
./.pio/build/native-emulation/program

# Build and run tests
pio test -e native-test

# Run specific test
pio test -e native-test -f test_hal_display
```

## Development Workflow

### 1. Develop on Host
- Use `native-emulation` for rapid development
- SDL2 window shows display output in real-time
- No hardware flashing required
- Faster iteration cycles

### 2. Test with Unit Tests
- Use `native-test` for automated testing
- Mock implementations capture operations
- Verify behavior without hardware dependencies

### 3. Validate on Hardware
- Use `esp32-hardware-debug` for hardware testing
- Real hardware validation
- Performance testing

### 4. Release
- Use `esp32-hardware-release` for production
- Optimized build
- Minimal resource usage

## Testing

### Unit Tests

Unit tests use mock implementations to verify HAL behavior:

```cpp
#include <unity.h>
#include "hal/hal_display.h"
#include "mocks/mock_display.h"

void test_display_init() {
    mock_display_set_init_result(true);
    
    bool result = hal_display_init();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(hal_display_is_initialized());
}

void setUp() {
    mock_display_reset();
}

void tearDown() {
    hal_display_deinit();
}
```

### Running Tests

```bash
# Run all tests
pio test -e native-test

# Run specific test file
pio test -e native-test -f test_hal_display

# Verbose test output
pio test -e native-test -v
```

## Adding New HAL Modules

### 1. Create Interface Header

```cpp
// firmware/include/hal/hal_newmodule.h
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool hal_newmodule_init(void);
void hal_newmodule_deinit(void);
// ... other functions

#ifdef __cplusplus
}
#endif
```

### 2. Implement ESP32 Version

```cpp
// firmware/src/hal/esp32/hal_newmodule_esp32.cpp
#include "hal/hal_newmodule.h"

#ifdef PLATFORM_ESP32

extern "C" {

bool hal_newmodule_init(void) {
    // ESP32-specific implementation
    return true;
}

// ... other functions

} // extern "C"

#endif // PLATFORM_ESP32
```

### 3. Implement Host Version

```cpp
// firmware/src/hal/host/hal_newmodule_host.cpp
#include "hal/hal_newmodule.h"

#ifdef PLATFORM_HOST

extern "C" {

bool hal_newmodule_init(void) {
    // Host-specific implementation
    return true;
}

// ... other functions

} // extern "C"

#endif // PLATFORM_HOST
```

### 4. Create Unit Tests

```cpp
// firmware/test/test_hal_newmodule.cpp
#include <unity.h>
#include "hal/hal_newmodule.h"

void test_newmodule_init() {
    bool result = hal_newmodule_init();
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_newmodule_init);
    return UNITY_END();
}
```

## Debugging

### Host Platform Debugging

```bash
# Build with debug symbols
pio run -e native-emulation

# Run with GDB
gdb ./.pio/build/native-emulation/program

# Run with Valgrind (Linux)
valgrind --leak-check=full ./.pio/build/native-emulation/program
```

### ESP32 Hardware Debugging

```bash
# Build debug version
pio run -e esp32-hardware-debug

# Upload and monitor
pio run -e esp32-hardware-debug -t upload -t monitor

# Use ESP32 debugger (if available)
pio debug -e esp32-hardware-debug
```

## Performance Considerations

### Memory Usage
- HAL adds minimal overhead (~1-2KB RAM)
- Function call overhead is negligible
- No dynamic allocation in HAL core

### Execution Speed
- HAL functions are thin wrappers
- Compiler optimization eliminates most overhead
- Critical paths can bypass HAL if needed

### Code Size
- Conditional compilation excludes unused platforms
- Release builds optimize out debug code
- Typical overhead: ~5-10KB flash

## Best Practices

### 1. Use HAL Consistently
```cpp
// Good: Use HAL functions
hal_display_clear(HAL_COLOR_BLACK);
hal_system_delay_ms(100);

// Avoid: Direct hardware access
// display.fillScreen(0x0000);  // Don't do this
// delay(100);                  // Don't do this
```

### 2. Handle Initialization Errors
```cpp
if (!hal_display_init()) {
    hal_system_log(HAL_LOG_LEVEL_ERROR, "APP", "Display init failed");
    return false;
}
```

### 3. Use Platform Flags Sparingly
```cpp
// Good: HAL abstracts platform differences
hal_system_log(HAL_LOG_LEVEL_INFO, "APP", "Message");

// Avoid: Platform-specific code in application
#ifdef PLATFORM_ESP32
    Serial.println("Message");
#else
    printf("Message\n");
#endif
```

### 4. Clean Up Resources
```cpp
// Always pair init/deinit calls
hal_display_init();
// ... use display
hal_display_deinit();
```

## Troubleshooting

### Common Issues

#### SDL2 Not Found (Host Build)
```bash
# Ubuntu/Debian
sudo apt-get install libsdl2-dev libsdl2-ttf-dev

# macOS
brew install sdl2 sdl2_ttf

# Windows
# Install SDL2 development libraries manually
```

#### Compilation Errors
- Check platform flags (`PLATFORM_ESP32` vs `PLATFORM_HOST`)
- Verify HAL implementations are included in build
- Check library dependencies in `platformio.ini`

#### Runtime Issues
- Verify HAL initialization order
- Check return values from HAL functions
- Enable debug logging for diagnostics

### Getting Help

1. Check HAL function return values
2. Enable verbose logging
3. Run unit tests to isolate issues
4. Compare behavior between platforms
5. Review HAL implementation source code

## Future Enhancements

- [ ] Complete audio HAL implementation
- [ ] Complete touch HAL implementation  
- [ ] Complete storage HAL implementation
- [ ] Add WiFi/Bluetooth HAL modules
- [ ] Add power management HAL
- [ ] Improve host platform simulation
- [ ] Add more comprehensive unit tests
- [ ] Add performance benchmarking
- [ ] Add HAL documentation generator

