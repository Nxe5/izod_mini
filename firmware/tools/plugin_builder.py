#!/usr/bin/env python3
"""
Izod Mini Plugin Builder
Builds plugins for the Izod Mini plugin system
"""

import os
import sys
import json
import subprocess
import argparse
import shutil
from pathlib import Path

class PluginBuilder:
    def __init__(self):
        self.script_dir = Path(__file__).parent
        self.firmware_dir = self.script_dir.parent
        self.plugins_dir = self.firmware_dir / "plugins"
        self.build_dir = self.firmware_dir / "build" / "plugins"
        
    def load_manifest(self, plugin_dir):
        """Load plugin manifest from JSON file"""
        manifest_path = plugin_dir / "manifest.json"
        if not manifest_path.exists():
            raise FileNotFoundError(f"No manifest.json found in {plugin_dir}")
        
        with open(manifest_path, 'r') as f:
            return json.load(f)
    
    def validate_manifest(self, manifest):
        """Validate plugin manifest"""
        required_fields = ['name', 'version', 'author', 'category', 'entry_point']
        for field in required_fields:
            if field not in manifest:
                raise ValueError(f"Missing required field: {field}")
        
        # Validate category
        if not isinstance(manifest['category'], int) or manifest['category'] < 0 or manifest['category'] > 5:
            raise ValueError("Category must be an integer between 0 and 5")
        
        # Validate compatibility flags
        if 'compatibility_flags' in manifest:
            if not isinstance(manifest['compatibility_flags'], int):
                raise ValueError("compatibility_flags must be an integer")
        
        # Validate memory requirement
        if 'memory_required' in manifest:
            if not isinstance(manifest['memory_required'], int) or manifest['memory_required'] < 0:
                raise ValueError("memory_required must be a positive integer")
        
        print(f"✓ Manifest validation passed for {manifest['name']}")
    
    def create_build_environment(self, plugin_name):
        """Create build environment for plugin"""
        plugin_build_dir = self.build_dir / plugin_name
        plugin_build_dir.mkdir(parents=True, exist_ok=True)
        
        # Create platformio.ini for plugin
        platformio_ini = plugin_build_dir / "platformio.ini"
        with open(platformio_ini, 'w') as f:
            f.write(f"""[env:plugin_{plugin_name}]
platform = espressif32
board = esp32-pico-v3-02
framework = arduino

build_flags = 
    -DPLUGIN_BUILD=1
    -DPLUGIN_NAME="{plugin_name}"
    -I../../include
    -fPIC
    -shared

lib_deps = 
    bblanchon/ArduinoJson@^6.21.3

build_type = release
""")
        
        return plugin_build_dir
    
    def build_plugin(self, plugin_dir, output_dir=None):
        """Build a single plugin"""
        plugin_name = plugin_dir.name
        print(f"Building plugin: {plugin_name}")
        
        # Load and validate manifest
        manifest = self.load_manifest(plugin_dir)
        self.validate_manifest(manifest)
        
        # Create build environment
        build_dir = self.create_build_environment(plugin_name)
        
        # Copy source files
        src_dir = build_dir / "src"
        src_dir.mkdir(exist_ok=True)
        
        if 'build' in manifest and 'source_files' in manifest['build']:
            source_files = manifest['build']['source_files']
        else:
            # Default to all .cpp and .c files
            source_files = list(plugin_dir.glob("*.cpp")) + list(plugin_dir.glob("*.c"))
            source_files = [f.name for f in source_files]
        
        for src_file in source_files:
            src_path = plugin_dir / src_file
            if src_path.exists():
                shutil.copy2(src_path, src_dir / src_file)
                print(f"  Copied: {src_file}")
            else:
                print(f"  Warning: Source file not found: {src_file}")
        
        # Copy manifest
        shutil.copy2(plugin_dir / "manifest.json", build_dir / "manifest.json")
        
        # Copy icon if exists
        icon_files = ['icon.bmp', 'icon.png', 'icon.jpg']
        for icon_file in icon_files:
            icon_path = plugin_dir / icon_file
            if icon_path.exists():
                shutil.copy2(icon_path, build_dir / icon_file)
                print(f"  Copied: {icon_file}")
                break
        
        # Build with PlatformIO
        print(f"  Compiling {plugin_name}...")
        try:
            result = subprocess.run([
                'pio', 'run', '-d', str(build_dir), '-e', f'plugin_{plugin_name}'
            ], capture_output=True, text=True, cwd=build_dir)
            
            if result.returncode != 0:
                print(f"  Build failed for {plugin_name}:")
                print(result.stderr)
                return False
            
            print(f"  ✓ Build successful for {plugin_name}")
            
            # Package plugin
            if output_dir:
                self.package_plugin(plugin_name, build_dir, Path(output_dir))
            
            return True
            
        except FileNotFoundError:
            print("  Error: PlatformIO not found. Please install PlatformIO first.")
            return False
        except Exception as e:
            print(f"  Build error: {e}")
            return False
    
    def package_plugin(self, plugin_name, build_dir, output_dir):
        """Package plugin for distribution"""
        output_dir.mkdir(parents=True, exist_ok=True)
        plugin_package_dir = output_dir / plugin_name
        plugin_package_dir.mkdir(exist_ok=True)
        
        # Copy manifest
        shutil.copy2(build_dir / "manifest.json", plugin_package_dir / "manifest.json")
        
        # Copy binary (if exists)
        binary_path = build_dir / ".pio" / "build" / f"plugin_{plugin_name}" / "firmware.bin"
        if binary_path.exists():
            shutil.copy2(binary_path, plugin_package_dir / "plugin.bin")
        
        # Copy icon (if exists)
        for icon_file in ['icon.bmp', 'icon.png', 'icon.jpg']:
            icon_path = build_dir / icon_file
            if icon_path.exists():
                shutil.copy2(icon_path, plugin_package_dir / icon_file)
                break
        
        # Create README
        readme_path = plugin_package_dir / "README.md"
        with open(readme_path, 'w') as f:
            manifest = json.load(open(build_dir / "manifest.json"))
            f.write(f"""# {manifest['name']}

**Version:** {manifest['version']}  
**Author:** {manifest['author']}  
**Description:** {manifest.get('description', 'No description available')}

## Installation

1. Copy this entire folder to `/Apps/` on your Izod Mini's SD card
2. The plugin will appear in the Apps menu
3. Select it to run

## Compatibility

- **API Version:** {manifest.get('api_version', 'Unknown')}
- **Memory Required:** {manifest.get('memory_required', 'Unknown')} bytes
- **Hardware Features:** {manifest.get('compatibility_flags', 'Unknown')}

## Files

- `manifest.json` - Plugin metadata
- `plugin.bin` - Compiled plugin binary
- `icon.*` - Plugin icon (if available)
""")
        
        print(f"  ✓ Plugin packaged: {plugin_package_dir}")
    
    def build_all_plugins(self, output_dir=None):
        """Build all plugins in the plugins directory"""
        if not self.plugins_dir.exists():
            print(f"Plugins directory not found: {self.plugins_dir}")
            return False
        
        success_count = 0
        total_count = 0
        
        for plugin_dir in self.plugins_dir.iterdir():
            if plugin_dir.is_dir() and (plugin_dir / "manifest.json").exists():
                total_count += 1
                if self.build_plugin(plugin_dir, output_dir):
                    success_count += 1
        
        print(f"\nBuild Summary: {success_count}/{total_count} plugins built successfully")
        return success_count == total_count
    
    def clean_build_dir(self):
        """Clean build directory"""
        if self.build_dir.exists():
            shutil.rmtree(self.build_dir)
            print(f"✓ Cleaned build directory: {self.build_dir}")
    
    def create_plugin_template(self, plugin_name, author="Unknown", category=5):
        """Create a new plugin template"""
        plugin_dir = self.plugins_dir / plugin_name
        plugin_dir.mkdir(parents=True, exist_ok=True)
        
        # Create manifest.json
        manifest = {
            "name": plugin_name.replace('_', ' ').title(),
            "version": "1.0.0",
            "author": author,
            "description": f"A new plugin: {plugin_name}",
            "category": category,
            "compatibility_flags": 4,  # TFT_DISPLAY
            "api_version": "1.0.0",
            "memory_required": 4096,
            "entry_point": f"{plugin_name}_main",
            "build": {
                "source_files": [f"{plugin_name}.cpp"],
                "include_dirs": ["../../include"],
                "libraries": []
            }
        }
        
        with open(plugin_dir / "manifest.json", 'w') as f:
            json.dump(manifest, f, indent=2)
        
        # Create source file template
        cpp_template = f'''/*
 * {manifest["name"]} Plugin
 * Generated by Izod Mini Plugin Builder
 */

#include "plugin_api.h"
#include <Arduino.h>

// Plugin state structure
typedef struct {{
    uint32_t counter;
    uint32_t last_update;
}} {plugin_name}_state_t;

// Plugin functions
static bool {plugin_name}_init(plugin_context_t* ctx);
static void {plugin_name}_run(plugin_context_t* ctx);
static void {plugin_name}_cleanup(plugin_context_t* ctx);
static bool {plugin_name}_event_handler(plugin_context_t* ctx, uint32_t event, void* data);

// Plugin manifest
static plugin_manifest_t {plugin_name}_manifest = PLUGIN_MANIFEST(
    "{manifest["name"]}",
    "{manifest["version"]}", 
    "{manifest["author"]}",
    static_cast<plugin_category_t>({category}),
    {plugin_name}_init,
    {plugin_name}_run,
    {plugin_name}_cleanup
);

// Plugin initialization
static bool {plugin_name}_init(plugin_context_t* ctx) {{
    if (!ctx) return false;
    
    // Allocate plugin state
    {plugin_name}_state_t* state = ({plugin_name}_state_t*)malloc(sizeof({plugin_name}_state_t));
    if (!state) return false;
    
    // Initialize state
    state->counter = 0;
    state->last_update = 0;
    
    // Store state in context
    ctx->private_data = state;
    ctx->private_data_size = sizeof({plugin_name}_state_t);
    
    const plugin_hal_t* hal = plugin_get_hal();
    if (hal && hal->system) {{
        hal->system->log("INFO", "{manifest["name"]} plugin initialized");
    }}
    
    return true;
}}

// Plugin main loop
static void {plugin_name}_run(plugin_context_t* ctx) {{
    if (!ctx || !ctx->private_data) return;
    
    {plugin_name}_state_t* state = ({plugin_name}_state_t*)ctx->private_data;
    const plugin_hal_t* hal = plugin_get_hal();
    
    if (!hal) return;
    
    uint32_t now = hal->system->get_time_ms();
    
    // Update display every 100ms
    if (now - state->last_update > 100) {{
        // Clear display
        hal->display->clear(PLUGIN_COLOR_BLACK);
        
        // Draw plugin content
        hal->display->text(10, 10, "{manifest["name"]}", PLUGIN_COLOR_WHITE, 2);
        
        char counter_text[32];
        snprintf(counter_text, sizeof(counter_text), "Counter: %lu", state->counter);
        hal->display->text(10, 40, counter_text, PLUGIN_COLOR_GREEN, 1);
        
        // Update display
        hal->display->update();
        
        state->last_update = now;
        state->counter++;
    }}
    
    // Small delay
    hal->system->delay_ms(10);
}}

// Plugin cleanup
static void {plugin_name}_cleanup(plugin_context_t* ctx) {{
    if (!ctx) return;
    
    const plugin_hal_t* hal = plugin_get_hal();
    if (hal && hal->system) {{
        hal->system->log("INFO", "{manifest["name"]} plugin cleanup");
    }}
    
    if (ctx->private_data) {{
        free(ctx->private_data);
        ctx->private_data = nullptr;
        ctx->private_data_size = 0;
    }}
}}

// Plugin event handler
static bool {plugin_name}_event_handler(plugin_context_t* ctx, uint32_t event, void* data) {{
    // Handle events here
    return false; // Event not handled
}}

// Plugin entry point
extern "C" {{
    const plugin_manifest_t* plugin_get_manifest(void) {{
        {plugin_name}_manifest.event_handler = {plugin_name}_event_handler;
        {plugin_name}_manifest.compatibility_flags = PLUGIN_COMPAT_TFT_DISPLAY;
        {plugin_name}_manifest.memory_required = 4096;
        
        strncpy((char*){plugin_name}_manifest.description, 
                "{manifest["description"]}", 
                PLUGIN_DESC_MAX_LENGTH - 1);
        
        return &{plugin_name}_manifest;
    }}
}}
'''
        
        with open(plugin_dir / f"{plugin_name}.cpp", 'w') as f:
            f.write(cpp_template)
        
        print(f"✓ Plugin template created: {plugin_dir}")
        print(f"  Edit {plugin_dir}/{plugin_name}.cpp to implement your plugin")
        print(f"  Edit {plugin_dir}/manifest.json to update metadata")

def main():
    parser = argparse.ArgumentParser(description="Izod Mini Plugin Builder")
    parser.add_argument('--build', metavar='PLUGIN', help='Build specific plugin')
    parser.add_argument('--build-all', action='store_true', help='Build all plugins')
    parser.add_argument('--clean', action='store_true', help='Clean build directory')
    parser.add_argument('--output', metavar='DIR', help='Output directory for packaged plugins')
    parser.add_argument('--create', metavar='NAME', help='Create new plugin template')
    parser.add_argument('--author', metavar='AUTHOR', default='Unknown', help='Plugin author name')
    parser.add_argument('--category', metavar='NUM', type=int, default=5, help='Plugin category (0-5)')
    
    args = parser.parse_args()
    
    builder = PluginBuilder()
    
    if args.clean:
        builder.clean_build_dir()
    
    if args.create:
        builder.create_plugin_template(args.create, args.author, args.category)
    
    if args.build:
        plugin_dir = builder.plugins_dir / args.build
        if not plugin_dir.exists():
            print(f"Plugin directory not found: {plugin_dir}")
            sys.exit(1)
        
        success = builder.build_plugin(plugin_dir, args.output)
        sys.exit(0 if success else 1)
    
    if args.build_all:
        success = builder.build_all_plugins(args.output)
        sys.exit(0 if success else 1)
    
    if not any([args.build, args.build_all, args.clean, args.create]):
        parser.print_help()

if __name__ == '__main__':
    main()

