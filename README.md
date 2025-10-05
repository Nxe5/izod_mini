# 🎵 Izod Mini - Open Source Music Player

A tiny iPod nano 3rd gen-like music player with advanced features, built around the ESP32-PICO-V3-02 microcontroller.

## ✨ Features

### 🎵 Core Music Player
- **High-Quality Audio**: PCM5102A DAC with 44.1kHz CD-quality playback
- **Touch Interface**: MPR121 capacitive touch wheel with 5-level sensitivity
- **2" TFT Display**: 240x320 color display for album art and UI
- **SD Card Support**: Expandable storage for music library
- **Physical Controls**: 5 buttons (Previous, Play/Pause, Next, Menu, Select)

### 🔌 Plugin System
- **WiFi Deauther**: Network penetration testing tool
- **WiFi Scanner**: Network discovery and analysis
- **BLE Scanner**: Bluetooth Low Energy device scanning
- **System Monitor**: Real-time device diagnostics
- **Evil Portal**: Captive portal attack tool with 20+ templates

### 🖥️ Development & Testing
- **Digital Emulation**: Test firmware on your computer without hardware
- **Hardware Abstraction Layer (HAL)**: Clean separation of hardware and software
- **Automated Testing**: Unit tests and CI/CD pipeline
- **Plugin Development**: Easy-to-use plugin API and tools

## 🚀 Quick Start

### Digital Testing (No Hardware Required)
```bash
cd firmware
pio run -e native-emulation
./.pio/build/native-emulation/program
```

### Building for Hardware
```bash
cd firmware
pio run -e esp32-hardware
```

### Deploying to Device
```bash
cd firmware
./scripts/deploy.sh -p /dev/ttyUSB0  # Replace with your port
```

## 📁 Project Structure

```
izod_mini/
├── firmware/           # ESP32 firmware and plugins
│   ├── src/           # Source code
│   ├── include/       # Headers and configuration
│   ├── plugins/       # Plugin applications
│   ├── test/          # Unit tests
│   └── scripts/       # Build and deployment tools
├── hardware/          # PCB designs and documentation
│   ├── main-pcb/      # Main board (ESP32, audio, display)
│   └── flex-pcb/      # Touch wheel and buttons
├── mechanical/        # 3D housing and component renders
├── docs/             # Project documentation
└── .github/          # CI/CD workflows
```

## 🔧 Hardware Specifications

### Main PCB
- **MCU**: ESP32-PICO-V3-02 (240MHz, 4MB Flash)
- **Audio**: PCM5102A DAC (I2S, 44.1kHz)
- **Display**: 2" TFT (240x320, SPI)
- **Touch**: MPR121 capacitive sensor (I2C)
- **Storage**: SD Card (SDMMC 4-bit)
- **Power**: TPS63070RNMR buck-boost converter

### Flex PCB
- **Touch Wheel**: MPR121 with 12 electrodes
- **Buttons**: 5 physical buttons with debouncing
- **Connector**: Flexible connection to main board

## 📚 Documentation

- **[Deployment Guide](DEPLOYMENT.md)** - How to build, test, and deploy firmware
- **[Hardware Documentation](hardware/)** - PCB designs, schematics, and BOMs
- **[Plugin Development](firmware/plugins/)** - Creating custom applications
- **[API Reference](firmware/include/)** - HAL and plugin APIs

## 🏷️ Releases

This project uses automated releases with semantic versioning:

- **v1.0.0** - Major releases (breaking changes)
- **v1.1.0** - Minor releases (new features)
- **v1.0.1** - Patch releases (bug fixes)

Each release includes:
- ✅ Signed firmware binaries
- ✅ SHA256 and MD5 checksums
- ✅ Installation instructions
- ✅ Hardware compatibility matrix

## 🤝 Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Workflow
1. **Fork** the repository
2. **Create** a feature branch
3. **Test** your changes with emulation
4. **Submit** a pull request

### Plugin Development
```bash
cd firmware/tools
python plugin_builder.py create my_plugin
```

## 🔒 Security

- **Firmware Signing**: Ed25519 and GPG signatures
- **Secure Boot**: ESP32 secure boot support
- **Encrypted Storage**: NVS encryption for sensitive data
- **Penetration Testing**: Built-in security tools

## 📄 License

This project is licensed under the MIT License - see [LICENSE](LICENSE) for details.

## 🙏 Acknowledgments

- **Flipper Zero** - Plugin system inspiration
- **M5Stack** - Hardware design references
- **Adafruit** - Library ecosystem
- **ESP32 Community** - Hardware support

## 🗺️ Roadmap

- [ ] **v1.0.0** - Core music player functionality
- [ ] **v1.1.0** - Advanced audio features (EQ, effects)
- [ ] **v1.2.0** - Wireless connectivity (WiFi, Bluetooth)
- [ ] **v2.0.0** - Multi-device synchronization
- [ ] **v2.1.0** - Mobile app companion

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/yourusername/izod_mini/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/izod_mini/discussions)
- **Documentation**: [Project Wiki](https://github.com/yourusername/izod_mini/wiki)

---

**Built with ❤️ for the open source hardware community**

*Making music portable, hackable, and fun!* 🎵📱
