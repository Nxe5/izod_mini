# Flex PCB - Touch Interface

The flex PCB provides the capacitive touch interface for the Izod Mini, featuring 5 touch buttons and an MPR121 touch controller.

## Components

### Touch Controller
- **MPR121**: 12-channel capacitive touch sensor controller
- **Touch Pads**: 5 capacitive touch buttons for user interface
- **Connector**: Interface to main PCB

## Design Files

- **KiCad Project**: `kicad/izod-mini-flex.kicad_pro`
- **Schematic**: `kicad/izod-mini-flex.kicad_sch`
- **PCB Layout**: `kicad/izod-mini-flex.kicad_pcb`

## Manufacturing

- **Gerber Files**: Located in `gerbers/` directory
- **BOM**: Bill of materials in `bom/` directory
- **Assembly**: Assembly guides in `../assembly/` directory

## Design Notes

- Flexible PCB design for curved surface mounting
- Optimized touch pad geometry for reliable detection
- Minimal component count for cost efficiency
- Robust connector design for reliable connection to main PCB

## Touch Button Layout

The 5 touch buttons are arranged to match the iPod nano 3rd gen interface:
- **Center Button**: Play/Pause, Select
- **Up Button**: Volume Up, Previous Track
- **Down Button**: Volume Down, Next Track
- **Left Button**: Previous Track, Menu
- **Right Button**: Next Track, Forward

## Integration

The flex PCB connects to the main PCB via a flexible connector, allowing it to wrap around the device housing while maintaining reliable electrical connection.

