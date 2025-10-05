# Getting Started with Izod Mini

This guide will help you get started with the Izod Mini project, from understanding the design to building your own device.

## 📋 Prerequisites

### Software Requirements
- **KiCad 7.0+**: For hardware design and PCB layout
- **ESP-IDF or Arduino IDE**: For firmware development
- **3D Modeling Software**: Fusion 360, FreeCAD, or similar
- **Git**: For version control

### Hardware Requirements
- **Computer**: Windows, macOS, or Linux
- **USB-C Cable**: For programming and charging
- **Basic Electronics Tools**: Soldering iron, multimeter, etc.

## 🚀 Quick Start

### 1. Clone the Repository
```bash
git clone https://github.com/yourusername/izod_mini.git
cd izod_mini
```

### 2. Explore the Project Structure
```bash
# View the project structure
tree -L 3

# Or use ls to explore directories
ls -la
```

### 3. Hardware Design
```bash
# Open KiCad projects
cd hardware/main-pcb/kicad
kicad izod-mini-main.kicad_pro

cd ../../flex-pcb/kicad
kicad izod-mini-flex.kicad_pro
```

### 4. Firmware Development
```bash
# Set up ESP-IDF
cd firmware
idf.py menuconfig
idf.py build
```

## 🔧 Detailed Setup

### Hardware Development

#### KiCad Setup
1. **Install KiCad**: Download from [kicad.org](https://kicad.org)
2. **Open Projects**: Navigate to hardware directories
3. **Review Schematics**: Understand the circuit design
4. **Check PCB Layout**: Verify component placement
5. **Generate Gerbers**: For manufacturing

#### Component Sourcing
1. **Review BOM**: Check `hardware/*/bom/` directories
2. **Find Suppliers**: Use provided part numbers
3. **Order Components**: Consider lead times
4. **Verify Specifications**: Match datasheet requirements

### Firmware Development

#### ESP-IDF Setup
```bash
# Install ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32
. ./export.sh

# Configure project
cd firmware
idf.py menuconfig
```

#### Arduino IDE Setup
1. **Install Arduino IDE**: Download from [arduino.cc](https://arduino.cc)
2. **Add ESP32 Support**: Use Board Manager
3. **Install Libraries**: Check `firmware/libs/` directory
4. **Configure Board**: Select ESP32-PICO-V3-02

### Mechanical Design

#### 3D Modeling
1. **Choose Software**: Fusion 360, FreeCAD, or SolidWorks
2. **Open Files**: Check `mechanical/housing/` directory
3. **Review Design**: Understand assembly requirements
4. **Modify as Needed**: Customize for your needs

#### 3D Printing
1. **Prepare Files**: Export STL from CAD software
2. **Slice Models**: Use Cura, PrusaSlicer, or similar
3. **Print Parts**: Follow recommended settings
4. **Post-Process**: Sand and finish as needed

## 🏭 Manufacturing

### PCB Manufacturing
1. **Generate Gerbers**: From KiCad projects
2. **Choose Manufacturer**: JLCPCB, PCBWay, or similar
3. **Upload Files**: Gerber files and drill files
4. **Review Quote**: Check specifications and pricing
5. **Place Order**: Wait for delivery

### Component Assembly
1. **Prepare Workspace**: Clean, well-lit area
2. **Organize Components**: Sort by BOM
3. **Follow Assembly Guide**: Check `hardware/assembly/`
4. **Test Continuously**: Verify connections
5. **Final Testing**: Complete functionality test

## 🧪 Testing

### Hardware Testing
1. **Continuity Check**: Verify all connections
2. **Power Supply Test**: Check voltage levels
3. **Component Test**: Verify IC functionality
4. **Interface Test**: Test connectors and buttons

### Firmware Testing
1. **Basic Boot**: Verify ESP32 starts
2. **Peripheral Test**: Test each component
3. **Audio Test**: Verify audio output
4. **Touch Test**: Verify touch interface
5. **Display Test**: Verify display functionality

## 🐛 Troubleshooting

### Common Issues

#### Hardware Issues
- **No Power**: Check battery and power supply
- **Audio Problems**: Verify DAC connections
- **Touch Not Working**: Check MPR121 connections
- **Display Issues**: Verify TFT connections

#### Firmware Issues
- **Boot Problems**: Check ESP32 configuration
- **Compilation Errors**: Verify ESP-IDF setup
- **Runtime Errors**: Check serial output
- **Performance Issues**: Optimize code

#### Mechanical Issues
- **Poor Fit**: Check tolerances and dimensions
- **Assembly Problems**: Follow assembly guide
- **Button Issues**: Verify clearances
- **Display Alignment**: Check mounting

### Getting Help
1. **Check Documentation**: Review relevant docs
2. **Search Issues**: Look for similar problems
3. **Ask Community**: Use GitHub Discussions
4. **Report Bugs**: Create detailed issue reports

## 📚 Next Steps

### Learning Resources
- **KiCad Tutorials**: Learn PCB design
- **ESP32 Documentation**: Master microcontroller
- **3D Modeling**: Improve CAD skills
- **Audio Engineering**: Understand audio systems

### Project Extensions
- **Custom Firmware**: Add new features
- **Hardware Mods**: Improve design
- **Mechanical Variants**: Create different housings
- **Accessories**: Design cases, stands, etc.

### Contributing
1. **Fork Repository**: Create your own copy
2. **Make Changes**: Implement improvements
3. **Test Thoroughly**: Verify functionality
4. **Submit PR**: Share your contributions

## 📞 Support

- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: Community chat and questions
- **Documentation**: Comprehensive guides in `docs/`
- **Wiki**: Additional resources and tutorials

## 🎯 Success Criteria

You'll know you're successful when:
- ✅ Hardware assembles correctly
- ✅ Firmware boots and runs
- ✅ Audio plays clearly
- ✅ Touch interface responds
- ✅ Display shows content
- ✅ Battery charges properly
- ✅ All features work as expected

Welcome to the Izod Mini community! 🎵

