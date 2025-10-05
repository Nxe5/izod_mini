# Izod Mini Firmware

ESP32-PICO-V3-02 based firmware for the Izod Mini music player with Flipper Zero-style plugin system.

## Features

### Core Features
- **High-Quality Audio**: PCM5102A DAC for excellent sound reproduction
- **Touch Interface**: MPR121-based capacitive touch wheel with 5-level sensitivity
- **Vibrant Display**: ST7789 240x320 TFT with optimized graphics
- **Expandable Storage**: SD card support with organized file system
- **Physical Controls**: 5 tactile buttons for reliable navigation

### Advanced Features
- **Plugin System**: Flipper Zero-inspired dynamic plugin loading
- **Touch Calibration**: Runtime sensitivity adjustment and calibration
- **Hardware Validation**: Comprehensive hardware testing and diagnostics
- **Secure Updates**: Ed25519 signed firmware with OTA support
- **Power Management**: Optimized for battery operation

## Hardware Configuration

### Target Platform
- **MCU**: ESP32-PICO-V3-02 (4MB Flash, WiFi, Bluetooth Classic)
- **Display**: ST7789 240x320 TFT via SPI
- **Touch**: MPR121 12-electrode capacitive touch wheel via I2C
- **Audio**: PCM5102A DAC via I2S
- **Storage**: SD card via SDMMC (4-bit mode)
- **Buttons**: 5 physical buttons (Previous, Play/Pause, Next, Menu, Select)

### Pin Assignments
See `include/hardware_config.h` for complete pin mapping:

```cpp
// Key pin assignments (ESP32-PICO-V3-02)
#define I2C_SDA_PIN            22       // MPR121 touch controller
#define I2C_SCL_PIN            19
#define TFT_CS_PIN             33       // ST7789 display
#define TFT_SCK_PIN            25
#define TFT_MOSI_PIN           26
#define TFT_DC_PIN             32
#define PCM_BCK_PIN            13       // PCM5102A audio DAC
#define PCM_DIN_PIN            15
#define PCM_LRCK_PIN           2
#define SD_CLK_PIN             6        // SD card (SDMMC mode)
#define SD_CMD_PIN             11
#define SD_DAT0_PIN            7
```

## Development Setup

### Prerequisites
- **PlatformIO**: Primary development environment
- **Python 3.8+**: For build tools and utilities
- **Git**: Version control

### Installation
```bash
# Clone repository
git clone https://github.com/yourusername/izod_mini.git
cd izod_mini/firmware

# Install PlatformIO
pip install platformio

# Build firmware
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

### Build Configurations
- **Standard** (`esp32-pico-v3-02`): Balanced performance and features
- **Debug** (`esp32-pico-v3-02-debug`): Development with verbose logging
- **Release** (`esp32-pico-v3-02-release`): Optimized for production

## Touch Sensitivity System

### 5-Level Sensitivity
The touch system supports 5 sensitivity levels:
1. **Very Low**: Least sensitive (high thresholds)
2. **Low**: Reduced sensitivity
3. **Medium**: Default balanced setting
4. **High**: Increased sensitivity
5. **Very High**: Most sensitive (low thresholds)

### Per-Electrode Customization
- Individual threshold adjustment for each of 12 electrodes
- Special compensation for smaller pads
- Runtime calibration and drift detection
- Persistent settings storage in NVS

### Calibration Tools
```bash
# Interactive sensitivity tuning
python tools/sensitivity_tuner.py --port /dev/ttyUSB0

# Set sensitivity level
python tools/sensitivity_tuner.py --port /dev/ttyUSB0 --level 4

# Auto-tune sensitivity
python tools/sensitivity_tuner.py --port /dev/ttyUSB0 --auto-tune
```

## Plugin System

### Architecture
The plugin system is inspired by Flipper Zero's architecture:
- **Dynamic Loading**: Plugins loaded from SD card at runtime
- **Hardware Abstraction**: Unified API for display, audio, input, storage
- **Lifecycle Management**: Init, run, suspend, resume, cleanup
- **Event System**: Button presses, touch events, system events
- **Memory Management**: Isolated plugin memory spaces

### Plugin Development
```bash
# Create new plugin template
python tools/plugin_builder.py --create my_plugin --author "Your Name"

# Build specific plugin
python tools/plugin_builder.py --build my_plugin --output /path/to/sd/Apps

# Build all plugins
python tools/plugin_builder.py --build-all --output /path/to/sd/Apps
```

### Plugin Structure
```
/Apps/my_plugin/
├── manifest.json          # Plugin metadata
├── plugin.bin             # Compiled plugin binary
├── icon.bmp              # Plugin icon (optional)
└── README.md             # Plugin documentation
```

### Plugin API Example
```cpp
#include "plugin_api.h"

static bool my_plugin_init(plugin_context_t* ctx) {
    // Initialize plugin state
    return true;
}

static void my_plugin_run(plugin_context_t* ctx) {
    const plugin_hal_t* hal = plugin_get_hal();
    
    // Clear display
    hal->display->clear(PLUGIN_COLOR_BLACK);
    
    // Draw content
    hal->display->text(10, 10, "Hello World!", PLUGIN_COLOR_WHITE, 2);
    hal->display->update();
    
    // Small delay
    hal->system->delay_ms(10);
}

static void my_plugin_cleanup(plugin_context_t* ctx) {
    // Cleanup resources
}

// Plugin manifest
static const plugin_manifest_t manifest = PLUGIN_MANIFEST(
    "My Plugin", "1.0.0", "Author Name",
    PLUGIN_CATEGORY_UTILITY,
    my_plugin_init, my_plugin_run, my_plugin_cleanup
);

PLUGIN_ENTRY_POINT(manifest);
```

## File System Organization

The firmware creates an organized file system on the SD card:

```
/
├── Music/                 # Music files (MP3, WAV, FLAC)
├── Playlists/            # Playlist files
├── Apps/                 # Plugin applications
│   ├── hello_world/      # Example plugin
│   ├── wifi_scanner/     # WiFi analysis plugin
│   └── audio_visualizer/ # Audio visualization plugin
├── Pentest/              # Penetration testing tools
│   ├── wifi/            # WiFi captures and configs
│   ├── ble/             # Bluetooth analysis
│   ├── lf_rfid/         # LF RFID dumps
│   └── scripts/         # Custom scripts
├── System/               # System files
│   ├── library.json     # Music library index
│   ├── config.json      # User configuration
│   └── thumbs/          # Album art thumbnails
└── Firmware/             # Firmware updates
    ├── current.bin       # Current firmware backup
    └── updates/          # Downloaded updates
```

## Hardware Validation

The firmware includes comprehensive hardware validation:

### Automatic Validation
- Pin conflict detection
- I2C bus scanning
- SPI bus initialization
- Component detection (MPR121, SD card, etc.)
- Button functionality testing

### Manual Testing
```cpp
// Run hardware self-test
hardware_run_self_test();

// Print diagnostics
hardware_print_diagnostics();

// Validate configuration
bool valid = hardware_config_validate();
```

### Validation Output
```
=== Hardware Validation ===
✓ Pin assignment validation passed
✓ I2C Bus: Found 1 I2C device(s)
✓ SPI Bus: SPI bus initialized
✓ MPR121 Touch: MPR121 detected and initialized
✓ SD Card: SD card detected (32768 MB)
✓ Display: Display pins configured
✓ Audio DAC: Audio DAC pins configured
✓ Buttons: All buttons responsive
✓ Power Management: Power management pins configured

=== Overall Result: PASS ===
```

## Configuration Management

### Touch Configuration
```cpp
// Set global sensitivity level
touch_sensitivity_manager_set_level(4);

// Set custom electrode threshold
touch_sensitivity_manager_set_electrode_threshold(11, 6, 3);

// Force calibration
touch_calibration_force_recalibration();
```

### Hardware Features
```cpp
// Check feature availability
if (hardware_feature_available(HW_FEATURE_TOUCH_WHEEL)) {
    // Touch wheel is available
}

// Get hardware version
const char* version = hardware_get_version_string();
```

## Serial Commands

The firmware supports various serial commands for debugging and configuration:

### Touch Commands
- `T` - Show touch status
- `S1-S5` - Set sensitivity level (1-5)
- `C` - Force calibration
- `E<electrode>,<touch>,<release>` - Set electrode threshold
- `R` - Reset to defaults

### Audio Commands
- `p` - Play/pause audio
- `+/-` - Volume up/down
- `m` - Play MP3 from /Music
- `w` - Play WAV from /Music
- `q` - Stop audio

### System Commands
- `F` - Create SD card folder structure
- `S` - List SD card files
- `X` - Quick format SD card

## Power Management

### Sleep Modes
- **Light Sleep**: Display off, audio continues
- **Deep Sleep**: System suspended, wake on button press
- **Power Off**: Complete shutdown via sleep switch

### Battery Monitoring
- Real-time battery level monitoring
- Low battery warnings
- Charging status detection
- Power consumption optimization

## Security Features

### Firmware Signing
- Ed25519 digital signatures for firmware integrity
- Public key embedded in firmware for verification
- Secure boot process with signature validation

### Plugin Security
- Plugin manifest validation
- Memory isolation between plugins
- API permission system
- Sandboxed execution environment

## Troubleshooting

### Common Issues

#### Build Errors
```bash
# Clean build directory
pio run --target clean

# Update PlatformIO
pio upgrade

# Check library dependencies
pio lib list
```

#### Hardware Issues
```bash
# Run hardware validation
# Connect via serial monitor and check validation output

# Test individual components
python tools/sensitivity_tuner.py --port /dev/ttyUSB0 --status
```

#### Touch Sensitivity Problems
```bash
# Auto-tune sensitivity
python tools/sensitivity_tuner.py --port /dev/ttyUSB0 --auto-tune

# Reset to defaults
python tools/sensitivity_tuner.py --port /dev/ttyUSB0
# Then type 'r' in interactive mode
```

### Debug Output
Enable verbose debugging in `platformio.ini`:
```ini
build_flags = 
    ${env:esp32-pico-v3-02.build_flags}
    -DDEBUG_MODE=1
    -DVERBOSE_LOGGING=1
    -DTOUCH_DEBUG_ENABLED=1
```

## Contributing

### Code Style
- Follow ESP32/Arduino coding conventions
- Use meaningful variable and function names
- Add comments for complex logic
- Keep functions focused and under 50 lines

### Testing
- Test on actual hardware before submitting
- Validate touch sensitivity across different conditions
- Ensure plugins load and run correctly
- Verify power consumption is reasonable

### Pull Requests
- Include detailed description of changes
- Test with multiple firmware configurations
- Update documentation as needed
- Follow the project's contribution guidelines

## License

This firmware is part of the Izod Mini project and is licensed under the MIT License. See the main project LICENSE file for details.

## Support

- **Issues**: Use GitHub Issues for bug reports
- **Discussions**: Use GitHub Discussions for questions
- **Documentation**: Check the `docs/` directory for detailed guides
- **Hardware**: See `hardware/` directory for PCB designs and BOMs