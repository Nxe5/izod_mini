#!/usr/bin/env python3
"""
Izod Mini Touch Sensitivity Tuner
Interactive tool for calibrating MPR121 touch sensitivity
"""

import serial
import time
import json
import argparse
import sys
from pathlib import Path

class TouchSensitivityTuner:
    def __init__(self, port, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.serial = None
        self.current_config = None
        
    def connect(self):
        """Connect to the Izod Mini device"""
        try:
            self.serial = serial.Serial(self.port, self.baudrate, timeout=1)
            time.sleep(2)  # Wait for device to initialize
            print(f"✓ Connected to Izod Mini on {self.port}")
            return True
        except serial.SerialException as e:
            print(f"✗ Failed to connect: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from device"""
        if self.serial:
            self.serial.close()
            print("✓ Disconnected")
    
    def send_command(self, command):
        """Send command to device and get response"""
        if not self.serial:
            return None
        
        self.serial.write(f"{command}\n".encode())
        time.sleep(0.1)
        
        response = []
        while self.serial.in_waiting:
            line = self.serial.readline().decode().strip()
            if line:
                response.append(line)
        
        return response
    
    def get_touch_status(self):
        """Get current touch status from device"""
        response = self.send_command("T")  # Touch status command
        if not response:
            return None
        
        # Parse touch status response
        status = {
            'electrodes': {},
            'sensitivity_level': 3,
            'calibration_time': 0
        }
        
        for line in response:
            if "Electrode" in line and ":" in line:
                # Parse electrode data
                parts = line.split()
                if len(parts) >= 6:
                    electrode = int(parts[0].replace('E', '').replace(':', ''))
                    status['electrodes'][electrode] = {
                        'enabled': parts[1] == 'Y',
                        'baseline': int(parts[2]),
                        'filtered': int(parts[3]),
                        'delta': int(parts[4]),
                        'touched': parts[5] == 'Y'
                    }
            elif "Global Level:" in line:
                parts = line.split()
                if len(parts) >= 3:
                    status['sensitivity_level'] = int(parts[2])
        
        return status
    
    def set_sensitivity_level(self, level):
        """Set global sensitivity level (1-5)"""
        if level < 1 or level > 5:
            print("Error: Sensitivity level must be between 1 and 5")
            return False
        
        response = self.send_command(f"S{level}")
        return "sensitivity changed" in str(response).lower()
    
    def set_electrode_threshold(self, electrode, touch_threshold, release_threshold):
        """Set custom threshold for specific electrode"""
        command = f"E{electrode},{touch_threshold},{release_threshold}"
        response = self.send_command(command)
        return "threshold set" in str(response).lower()
    
    def force_calibration(self):
        """Force touch calibration"""
        response = self.send_command("C")
        return "calibration" in str(response).lower()
    
    def interactive_tuning(self):
        """Interactive tuning session"""
        print("\n" + "="*60)
        print("Izod Mini Touch Sensitivity Tuner")
        print("="*60)
        print("Commands:")
        print("  1-5: Set global sensitivity level")
        print("  s: Show current status")
        print("  c: Force calibration")
        print("  e: Set electrode threshold")
        print("  a: Auto-tune sensitivity")
        print("  r: Reset to defaults")
        print("  q: Quit")
        print("="*60)
        
        while True:
            try:
                command = input("\nTouch Tuner> ").strip().lower()
                
                if command == 'q':
                    break
                elif command in ['1', '2', '3', '4', '5']:
                    level = int(command)
                    if self.set_sensitivity_level(level):
                        print(f"✓ Sensitivity level set to {level}")
                    else:
                        print("✗ Failed to set sensitivity level")
                
                elif command == 's':
                    self.show_status()
                
                elif command == 'c':
                    print("Forcing calibration...")
                    if self.force_calibration():
                        print("✓ Calibration started")
                        time.sleep(2)
                        self.show_status()
                    else:
                        print("✗ Failed to start calibration")
                
                elif command == 'e':
                    self.set_electrode_interactive()
                
                elif command == 'a':
                    self.auto_tune()
                
                elif command == 'r':
                    self.reset_to_defaults()
                
                elif command == 'help' or command == 'h':
                    self.show_help()
                
                else:
                    print("Unknown command. Type 'h' for help.")
                    
            except KeyboardInterrupt:
                print("\nExiting...")
                break
            except Exception as e:
                print(f"Error: {e}")
    
    def show_status(self):
        """Show current touch status"""
        status = self.get_touch_status()
        if not status:
            print("✗ Failed to get touch status")
            return
        
        print(f"\nCurrent Status:")
        print(f"Global Sensitivity Level: {status['sensitivity_level']}")
        print(f"Electrodes:")
        print("  ID | Enabled | Baseline | Filtered | Delta | Touched | Health")
        print("-----|---------|----------|----------|-------|---------|--------")
        
        for i in range(12):
            if i in status['electrodes']:
                e = status['electrodes'][i]
                health = self.assess_electrode_health(e)
                print(f"  {i:2d} |    {e['enabled'] and 'Y' or 'N'}    |   {e['baseline']:4d}   |   {e['filtered']:4d}   | {e['delta']:4d}  |    {e['touched'] and 'Y' or 'N'}    | {health}")
            else:
                print(f"  {i:2d} |    ?    |    ?     |    ?     |   ?   |    ?    |   ?")
    
    def assess_electrode_health(self, electrode_data):
        """Assess electrode health based on readings"""
        baseline = electrode_data['baseline']
        filtered = electrode_data['filtered']
        delta = electrode_data['delta']
        
        if baseline < 50:
            return "LOW"
        elif baseline > 1000:
            return "HIGH"
        elif abs(baseline - filtered) > 100:
            return "NOISY"
        elif delta > 200:
            return "STUCK"
        else:
            return "GOOD"
    
    def set_electrode_interactive(self):
        """Interactive electrode threshold setting"""
        try:
            electrode = int(input("Enter electrode number (0-11): "))
            if electrode < 0 or electrode > 11:
                print("Error: Electrode must be between 0 and 11")
                return
            
            touch_threshold = int(input("Enter touch threshold (1-255): "))
            if touch_threshold < 1 or touch_threshold > 255:
                print("Error: Touch threshold must be between 1 and 255")
                return
            
            release_threshold = int(input("Enter release threshold (1-255): "))
            if release_threshold < 1 or release_threshold > 255:
                print("Error: Release threshold must be between 1 and 255")
                return
            
            if release_threshold >= touch_threshold:
                print("Error: Release threshold must be less than touch threshold")
                return
            
            if self.set_electrode_threshold(electrode, touch_threshold, release_threshold):
                print(f"✓ Electrode {electrode} thresholds set: touch={touch_threshold}, release={release_threshold}")
            else:
                print("✗ Failed to set electrode thresholds")
                
        except ValueError:
            print("Error: Please enter valid numbers")
    
    def auto_tune(self):
        """Automatic sensitivity tuning"""
        print("Starting auto-tune process...")
        print("Please don't touch the device during calibration...")
        
        # Force calibration first
        if not self.force_calibration():
            print("✗ Failed to start calibration")
            return
        
        time.sleep(3)  # Wait for calibration to complete
        
        # Get baseline readings
        status = self.get_touch_status()
        if not status:
            print("✗ Failed to get status after calibration")
            return
        
        print("Analyzing electrode performance...")
        
        # Analyze each electrode and suggest thresholds
        suggestions = {}
        for electrode, data in status['electrodes'].items():
            if not data['enabled']:
                continue
            
            baseline = data['baseline']
            filtered = data['filtered']
            noise_level = abs(baseline - filtered)
            
            # Calculate recommended thresholds based on noise level
            recommended_touch = max(3, noise_level * 2 + 5)
            recommended_release = max(1, noise_level + 2)
            
            # Clamp to reasonable ranges
            recommended_touch = min(30, recommended_touch)
            recommended_release = min(15, recommended_release)
            
            suggestions[electrode] = {
                'touch': recommended_touch,
                'release': recommended_release,
                'noise': noise_level
            }
        
        # Show suggestions
        print("\nRecommended thresholds:")
        print("Electrode | Touch | Release | Noise Level")
        print("----------|-------|---------|------------")
        for electrode, suggestion in suggestions.items():
            print(f"    {electrode:2d}    |  {suggestion['touch']:3d}  |   {suggestion['release']:3d}   |     {suggestion['noise']:3d}")
        
        # Ask for confirmation
        apply = input("\nApply these settings? (y/n): ").strip().lower()
        if apply == 'y':
            success_count = 0
            for electrode, suggestion in suggestions.items():
                if self.set_electrode_threshold(electrode, suggestion['touch'], suggestion['release']):
                    success_count += 1
                    print(f"✓ Applied settings for electrode {electrode}")
                else:
                    print(f"✗ Failed to apply settings for electrode {electrode}")
            
            print(f"\nAuto-tune completed: {success_count}/{len(suggestions)} electrodes configured")
        else:
            print("Auto-tune cancelled")
    
    def reset_to_defaults(self):
        """Reset touch configuration to defaults"""
        confirm = input("Reset all touch settings to defaults? (y/n): ").strip().lower()
        if confirm == 'y':
            response = self.send_command("R")  # Reset command
            if "reset" in str(response).lower():
                print("✓ Touch configuration reset to defaults")
                time.sleep(1)
                self.show_status()
            else:
                print("✗ Failed to reset configuration")
    
    def show_help(self):
        """Show detailed help"""
        print("""
Touch Sensitivity Tuning Help:

Sensitivity Levels:
  1 - Very Low (least sensitive, high thresholds)
  2 - Low 
  3 - Medium (default, balanced)
  4 - High
  5 - Very High (most sensitive, low thresholds)

Electrode Health:
  GOOD  - Normal operation
  LOW   - Baseline too low (possible connection issue)
  HIGH  - Baseline too high (possible short circuit)
  NOISY - Excessive noise (interference or poor grounding)
  STUCK - High delta (electrode may be stuck/covered)

Tips:
- Start with global sensitivity levels before custom thresholds
- Use auto-tune for initial setup
- Monitor electrode health regularly
- Higher touch thresholds = less sensitive
- Release threshold should be lower than touch threshold
- Small pad (electrode 11) may need different settings
        """)
    
    def save_config(self, filename):
        """Save current configuration to file"""
        status = self.get_touch_status()
        if not status:
            print("✗ Failed to get current configuration")
            return False
        
        config = {
            'sensitivity_level': status['sensitivity_level'],
            'electrodes': {}
        }
        
        for electrode, data in status['electrodes'].items():
            config['electrodes'][electrode] = {
                'enabled': data['enabled'],
                'baseline': data['baseline']
            }
        
        try:
            with open(filename, 'w') as f:
                json.dump(config, f, indent=2)
            print(f"✓ Configuration saved to {filename}")
            return True
        except Exception as e:
            print(f"✗ Failed to save configuration: {e}")
            return False

def main():
    parser = argparse.ArgumentParser(description="Izod Mini Touch Sensitivity Tuner")
    parser.add_argument('--port', '-p', required=True, help='Serial port (e.g., /dev/ttyUSB0, COM3)')
    parser.add_argument('--baudrate', '-b', type=int, default=115200, help='Baud rate (default: 115200)')
    parser.add_argument('--level', '-l', type=int, choices=[1,2,3,4,5], help='Set sensitivity level and exit')
    parser.add_argument('--status', '-s', action='store_true', help='Show status and exit')
    parser.add_argument('--auto-tune', '-a', action='store_true', help='Run auto-tune and exit')
    parser.add_argument('--save', metavar='FILE', help='Save configuration to file')
    
    args = parser.parse_args()
    
    tuner = TouchSensitivityTuner(args.port, args.baudrate)
    
    if not tuner.connect():
        sys.exit(1)
    
    try:
        if args.level:
            if tuner.set_sensitivity_level(args.level):
                print(f"✓ Sensitivity level set to {args.level}")
            else:
                print("✗ Failed to set sensitivity level")
        
        elif args.status:
            tuner.show_status()
        
        elif args.auto_tune:
            tuner.auto_tune()
        
        elif args.save:
            tuner.save_config(args.save)
        
        else:
            tuner.interactive_tuning()
    
    finally:
        tuner.disconnect()

if __name__ == '__main__':
    main()

