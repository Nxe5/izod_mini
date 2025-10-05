# 🚀 Izod Mini Firmware Deployment Guide

This guide explains how to deploy firmware to your Izod Mini device and set up automated GitHub releases.

## 📋 Table of Contents

- [Local Development & Testing](#local-development--testing)
- [Deploying to Your Device](#deploying-to-your-device)
- [GitHub Releases & CI/CD](#github-releases--cicd)
- [Troubleshooting](#troubleshooting)

## 🖥️ Local Development & Testing

### Digital Testing (No Hardware Required)

You can test your firmware logic without physical hardware:

```bash
cd firmware

# Run interactive emulation
pio run -e native-emulation
./.pio/build/native-emulation/program

# Run automated tests
pio test -e native-test
```

This runs your firmware on your computer with simulated hardware, perfect for:
- ✅ UI development and testing
- ✅ Logic validation
- ✅ CI/CD testing
- ✅ Debugging without hardware

### Building for Hardware

```bash
cd firmware

# Build for ESP32 (when hardware issues are fixed)
pio run -e esp32-hardware

# Build debug version
pio run -e esp32-hardware-debug

# Build optimized release version
pio run -e esp32-hardware-release
```

## 🔧 Deploying to Your Device

### Method 1: Using the Deployment Script (Recommended)

```bash
cd firmware

# Show available ports and help
./scripts/deploy.sh --help

# Deploy to your device (replace with your port)
./scripts/deploy.sh -p /dev/ttyUSB0

# Deploy debug version with monitoring
./scripts/deploy.sh -e esp32-hardware-debug -p /dev/ttyUSB0 -m

# Build only (don't upload)
./scripts/deploy.sh --build-only
```

### Method 2: Manual PlatformIO Commands

```bash
cd firmware

# Build and upload
pio run -e esp32-hardware --target upload

# Upload to specific port
pio run -e esp32-hardware --target upload --upload-port /dev/ttyUSB0

# Start serial monitor
pio device monitor --port /dev/ttyUSB0 --baud 115200
```

### Method 3: Using ESP32 Flash Tool

1. Build the firmware: `pio run -e esp32-hardware`
2. Download ESP32 Flash Tool
3. Flash the `.pio/build/esp32-hardware/firmware.bin` file

## 🏷️ GitHub Releases & CI/CD

### Setting Up Automated Releases

The project includes a comprehensive GitHub Actions workflow that automatically:

1. **Builds firmware** for all environments (standard, debug, release)
2. **Runs tests** (unit tests + emulation)
3. **Creates releases** when you push version tags
4. **Signs firmware** with Ed25519 and GPG signatures
5. **Generates checksums** for integrity verification

### Creating a Release

1. **Commit your changes:**
   ```bash
   git add .
   git commit -m "Add new feature"
   ```

2. **Create and push a version tag:**
   ```bash
   # For a patch release (bug fixes)
   git tag v1.0.1
   git push origin v1.0.1
   
   # For a minor release (new features)
   git tag v1.1.0
   git push origin v1.1.0
   
   # For a major release (breaking changes)
   git tag v2.0.0
   git push origin v2.0.0
   ```

3. **GitHub Actions will automatically:**
   - Build all firmware variants
   - Run tests
   - Create a GitHub release
   - Sign the firmware
   - Generate checksums

### Release Types

- **`v1.0.1`** - Patch release (bug fixes)
- **`v1.1.0`** - Minor release (new features)
- **`v2.0.0`** - Major release (breaking changes)
- **`v1.0.0-alpha.1`** - Alpha pre-release
- **`v1.0.0-beta.1`** - Beta pre-release
- **`v1.0.0-rc.1`** - Release candidate

### Downloading Releases

1. Go to your GitHub repository
2. Click on "Releases" in the right sidebar
3. Download the firmware file for your needs:
   - `izod-mini-firmware-esp32-hardware-v1.0.0.bin` - Standard production build
   - `izod-mini-firmware-esp32-hardware-debug-v1.0.0.bin` - Debug build
   - `izod-mini-firmware-esp32-hardware-release-v1.0.0.bin` - Optimized build

### Verifying Firmware Integrity

Each release includes checksums and signatures:

```bash
# Verify SHA256 checksum
sha256sum -c checksums.sha256

# Verify GPG signature
gpg --verify izod-mini-firmware-esp32-hardware-v1.0.0.bin.sig.gpg

# Verify Ed25519 signature
openssl dgst -sha256 -verify ed25519_public.pem -signature firmware.sig firmware.bin
```

## 🔍 Troubleshooting

### Common Issues

#### 1. "No serial ports detected"
```bash
# Check if ESP32 is connected
ls /dev/tty* | grep -E "(USB|ACM)"

# On macOS
ls /dev/cu.*

# On Windows
# Check Device Manager for COM ports
```

#### 2. "Upload failed"
- Make sure ESP32 is in bootloader mode:
  1. Hold the BOOT button
  2. Press and release the RESET button
  3. Release the BOOT button
- Check if another application is using the port
- Try a different USB cable
- Check USB port permissions

#### 3. "Build failed"
- Make sure all dependencies are installed: `pio lib install`
- Check if hardware_config.h exists and has correct pin definitions
- Verify PlatformIO configuration in `platformio.ini`

#### 4. "Tests failing"
- Run tests individually: `pio test -e native-test -f test_*`
- Check if SDL2 is installed for emulation tests
- Verify HAL implementations are correct

### Getting Help

1. **Check the logs** in GitHub Actions for CI/CD issues
2. **Run with verbose output**: `pio run -e esp32-hardware -v`
3. **Check serial monitor** for runtime errors
4. **Verify hardware connections** match `hardware_config.h`

### Development Workflow

1. **Develop locally** using emulation: `pio run -e native-emulation`
2. **Test changes** with unit tests: `pio test -e native-test`
3. **Build for hardware** when ready: `pio run -e esp32-hardware`
4. **Deploy to device** for testing: `./scripts/deploy.sh -p /dev/ttyUSB0`
5. **Create release** when stable: `git tag v1.0.1 && git push origin v1.0.1`

## 📚 Additional Resources

- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP32 Development Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Hardware Configuration Guide](firmware/include/hardware_config.h)

## 🎯 Next Steps

1. **Fix hardware build issues** (pin definitions, missing includes)
2. **Test on actual hardware** once build is working
3. **Set up GitHub repository** and push your code
4. **Create your first release** with `git tag v1.0.0`
5. **Share with community** and get feedback!

---

**Happy coding! 🎵📱**
