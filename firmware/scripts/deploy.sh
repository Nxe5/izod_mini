#!/bin/bash

# Izod Mini Firmware Deployment Script
# This script helps you deploy firmware to your ESP32-PICO-V3-02 device

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
ENVIRONMENT="esp32-hardware"
PORT=""
BAUD_RATE="115200"
BUILD_ONLY=false
MONITOR=false

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -e, --environment ENV    Build environment (default: esp32-hardware)"
    echo "                           Options: esp32-hardware, esp32-hardware-debug, esp32-hardware-release"
    echo "  -p, --port PORT          Serial port (e.g., /dev/ttyUSB0, COM3)"
    echo "  -b, --baud BAUD          Baud rate (default: 115200)"
    echo "  -b, --build-only         Only build, don't upload"
    echo "  -m, --monitor            Start serial monitor after upload"
    echo "  -h, --help               Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 -p /dev/ttyUSB0                    # Build and upload to /dev/ttyUSB0"
    echo "  $0 -e esp32-hardware-debug -p COM3    # Build debug version and upload to COM3"
    echo "  $0 --build-only                       # Only build, don't upload"
    echo "  $0 -p /dev/ttyUSB0 -m                 # Upload and start monitor"
}

# Function to detect available ports
detect_ports() {
    print_status "Detecting available serial ports..."
    
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        ports=$(ls /dev/cu.* 2>/dev/null | grep -E "(usb|serial)" || true)
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # Linux
        ports=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || true)
    else
        # Windows (Git Bash)
        ports=$(ls /dev/ttyS* 2>/dev/null || true)
    fi
    
    if [ -z "$ports" ]; then
        print_warning "No serial ports detected. Make sure your ESP32 is connected."
        return 1
    fi
    
    echo "Available ports:"
    for port in $ports; do
        echo "  - $port"
    done
    return 0
}

# Function to build firmware
build_firmware() {
    print_status "Building firmware for environment: $ENVIRONMENT"
    
    if ! pio run -e "$ENVIRONMENT"; then
        print_error "Build failed!"
        exit 1
    fi
    
    print_success "Build completed successfully!"
    
    # Show build info
    BUILD_DIR=".pio/build/$ENVIRONMENT"
    if [ -f "$BUILD_DIR/firmware.bin" ]; then
        FIRMWARE_SIZE=$(stat -f%z "$BUILD_DIR/firmware.bin" 2>/dev/null || stat -c%s "$BUILD_DIR/firmware.bin" 2>/dev/null)
        print_status "Firmware size: $((FIRMWARE_SIZE / 1024)) KB"
        print_status "Firmware location: $BUILD_DIR/firmware.bin"
    fi
}

# Function to upload firmware
upload_firmware() {
    if [ -z "$PORT" ]; then
        print_error "No port specified. Use -p to specify a port or run without -p to see available ports."
        detect_ports
        exit 1
    fi
    
    print_status "Uploading firmware to $PORT..."
    
    if ! pio run -e "$ENVIRONMENT" --target upload --upload-port "$PORT"; then
        print_error "Upload failed!"
        print_warning "Make sure:"
        print_warning "  1. Your ESP32 is connected to $PORT"
        print_warning "  2. The port is not in use by another application"
        print_warning "  3. You have permission to access the port"
        print_warning "  4. The ESP32 is in bootloader mode (hold BOOT button while pressing RESET)"
        exit 1
    fi
    
    print_success "Upload completed successfully!"
}

# Function to start serial monitor
start_monitor() {
    if [ -z "$PORT" ]; then
        print_error "No port specified for monitoring."
        exit 1
    fi
    
    print_status "Starting serial monitor on $PORT at $BAUD_RATE baud..."
    print_warning "Press Ctrl+C to stop monitoring"
    
    pio device monitor --port "$PORT" --baud "$BAUD_RATE"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -e|--environment)
            ENVIRONMENT="$2"
            shift 2
            ;;
        -p|--port)
            PORT="$2"
            shift 2
            ;;
        -b|--baud)
            BAUD_RATE="$2"
            shift 2
            ;;
        --build-only)
            BUILD_ONLY=true
            shift
            ;;
        -m|--monitor)
            MONITOR=true
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Validate environment
case $ENVIRONMENT in
    esp32-hardware|esp32-hardware-debug|esp32-hardware-release)
        ;;
    *)
        print_error "Invalid environment: $ENVIRONMENT"
        print_error "Valid environments: esp32-hardware, esp32-hardware-debug, esp32-hardware-release"
        exit 1
        ;;
esac

# Main execution
print_status "Izod Mini Firmware Deployment"
print_status "Environment: $ENVIRONMENT"
print_status "Port: ${PORT:-"Not specified"}"
print_status "Baud Rate: $BAUD_RATE"

# Check if PlatformIO is installed
if ! command -v pio &> /dev/null; then
    print_error "PlatformIO is not installed or not in PATH"
    print_error "Install it with: pip install platformio"
    exit 1
fi

# Change to firmware directory
cd "$(dirname "$0")/.."

# Build firmware
build_firmware

# Upload firmware if not build-only
if [ "$BUILD_ONLY" = false ]; then
    upload_firmware
fi

# Start monitor if requested
if [ "$MONITOR" = true ]; then
    start_monitor
fi

print_success "Deployment completed!"
