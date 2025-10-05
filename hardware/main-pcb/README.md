# Main PCB - Izod Mini

The main PCB contains the core electronics for the Izod Mini music player, including the ESP32 microcontroller, audio processing, power management, and display interface.

## Components

### Microcontroller
- **ESP32-PICO-V3-02**: Main processor with WiFi/Bluetooth capabilities

### Audio Processing
- **PCM5102A**: High-quality stereo DAC for audio output
- **ADP150AUJZ-3.3-R7**: Low-noise LDO regulator for audio circuits

### Power Management
- **TPS63070RNMR**: Buck-boost converter for battery management
- **MAX17055EWL_T**: Fuel gauge for accurate battery monitoring
- **LT1763CS8-3.3_TR**: Low-dropout regulator for sensitive circuits
- **MCP73871-2CC**: Battery charging controller

### Connectivity
- **CUS-12TB**: USB-C connector for charging and data
- **CH9102F**: USB-to-serial converter for programming

### Display Interface
- Connector for 2" TFT display
- Interface to flex PCB with touch controls

## Design Files

- **KiCad Project**: `kicad/izod-mini-main.kicad_pro`
- **Schematic**: `kicad/izod-mini-main.kicad_sch`
- **PCB Layout**: `kicad/izod-mini-main.kicad_pcb`

## Manufacturing

- **Gerber Files**: Located in `gerbers/` directory
- **BOM**: Bill of materials in `bom/` directory
- **Assembly**: Assembly guides in `../assembly/` directory

## Design Notes

- Optimized for compact form factor
- Power management designed for long battery life
- Audio quality prioritized with dedicated power rails
- Robust USB-C implementation with proper ESD protection

