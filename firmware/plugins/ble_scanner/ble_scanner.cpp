/*
 * BLE Scanner Plugin
 * Bluetooth Low Energy device scanner and analyzer
 */

#include "plugin_api.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Plugin state
typedef struct {
    bool scanning;
    bool continuous_scan;
    int selected_device;
    int device_count;
    uint32_t last_scan_time;
    uint32_t scan_duration;
    
    // Device list
    struct {
        char name[33];
        char address[18];
        int32_t rssi;
        uint32_t first_seen;
        uint32_t last_seen;
        uint16_t appearance;
        bool has_name;
        bool connectable;
        uint8_t addr_type;
        uint16_t manufacturer_id;
        uint8_t service_count;
        char services[5][37]; // Up to 5 services, UUID string format
    } device_list[30];
    
    // UI state
    enum {
        STATE_MENU,
        STATE_SCANNING,
        STATE_DEVICE_LIST,
        STATE_DEVICE_DETAILS,
        STATE_SERVICES,
        STATE_SETTINGS
    } ui_state;
    
    // Settings
    uint16_t scan_time_seconds;
    bool show_unnamed;
    bool active_scan;
    
    // Statistics
    uint32_t total_scans;
    uint32_t unique_devices;
    uint32_t packets_received;
    
} ble_scanner_state_t;

// BLE scan callback class
class BLEScanCallback : public BLEAdvertisedDeviceCallbacks {
private:
    ble_scanner_state_t* state;
    
public:
    BLEScanCallback(ble_scanner_state_t* s) : state(s) {}
    
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (!state || state->device_count >= 30) return;
        
        state->packets_received++;
        
        // Check if device already exists
        std::string addr = advertisedDevice.getAddress().toString();
        for (int i = 0; i < state->device_count; i++) {
            if (strcmp(state->device_list[i].address, addr.c_str()) == 0) {
                // Update existing device
                state->device_list[i].rssi = advertisedDevice.getRSSI();
                state->device_list[i].last_seen = millis();
                return;
            }
        }
        
        // Add new device
        int idx = state->device_count;
        
        // Name
        if (advertisedDevice.haveName()) {
            strncpy(state->device_list[idx].name, advertisedDevice.getName().c_str(), 32);
            state->device_list[idx].name[32] = '\0';
            state->device_list[idx].has_name = true;
        } else {
            strcpy(state->device_list[idx].name, "<Unknown>");
            state->device_list[idx].has_name = false;
        }
        
        // Address
        strncpy(state->device_list[idx].address, addr.c_str(), 17);
        state->device_list[idx].address[17] = '\0';
        
        // RSSI
        state->device_list[idx].rssi = advertisedDevice.getRSSI();
        
        // Timestamps
        uint32_t now = millis();
        state->device_list[idx].first_seen = now;
        state->device_list[idx].last_seen = now;
        
        // Appearance
        if (advertisedDevice.haveAppearance()) {
            state->device_list[idx].appearance = advertisedDevice.getAppearance();
        } else {
            state->device_list[idx].appearance = 0;
        }
        
        // Address type and connectivity
        state->device_list[idx].addr_type = advertisedDevice.getAddressType();
        state->device_list[idx].connectable = advertisedDevice.isConnectable();
        
        // Manufacturer data
        if (advertisedDevice.haveManufacturerData()) {
            std::string mfgData = advertisedDevice.getManufacturerData();
            if (mfgData.length() >= 2) {
                state->device_list[idx].manufacturer_id = 
                    (uint8_t)mfgData[1] << 8 | (uint8_t)mfgData[0];
            }
        } else {
            state->device_list[idx].manufacturer_id = 0;
        }
        
        // Service UUIDs
        state->device_list[idx].service_count = 0;
        if (advertisedDevice.haveServiceUUID()) {
            // Note: ESP32 BLE library doesn't provide easy access to all service UUIDs
            // This would need more complex implementation to extract all services
            BLEUUID serviceUUID = advertisedDevice.getServiceUUID();
            strncpy(state->device_list[idx].services[0], serviceUUID.toString().c_str(), 36);
            state->device_list[idx].services[0][36] = '\0';
            state->device_list[idx].service_count = 1;
        }
        
        state->device_count++;
        state->unique_devices = state->device_count;
    }
};

// Function prototypes
static bool ble_scanner_init(plugin_context_t* ctx);
static void ble_scanner_run(plugin_context_t* ctx);
static void ble_scanner_cleanup(plugin_context_t* ctx);
static bool ble_scanner_event_handler(plugin_context_t* ctx, uint32_t event, void* data);

// Internal functions
static void start_ble_scan(ble_scanner_state_t* state);
static void stop_ble_scan(ble_scanner_state_t* state);
static void save_scan_results(ble_scanner_state_t* state, const plugin_hal_t* hal);
static void draw_menu(ble_scanner_state_t* state, const plugin_hal_t* hal);
static void draw_device_list(ble_scanner_state_t* state, const plugin_hal_t* hal);
static void draw_device_details(ble_scanner_state_t* state, const plugin_hal_t* hal);
static void draw_services(ble_scanner_state_t* state, const plugin_hal_t* hal);
static void draw_settings(ble_scanner_state_t* state, const plugin_hal_t* hal);
static const char* get_device_type_string(uint16_t appearance);
static const char* get_manufacturer_name(uint16_t id);

// Global BLE objects
static BLEScan* pBLEScan = nullptr;
static BLEScanCallback* pScanCallback = nullptr;

// Plugin manifest
static plugin_manifest_t ble_scanner_manifest = PLUGIN_MANIFEST(
    "BLE Scanner",
    "1.0.0",
    "Izod Mini Team",
    PLUGIN_CATEGORY_PENTEST,
    ble_scanner_init,
    ble_scanner_run,
    ble_scanner_cleanup
);

// Plugin initialization
static bool ble_scanner_init(plugin_context_t* ctx) {
    if (!ctx) return false;
    
    // Allocate plugin state
    ble_scanner_state_t* state = (ble_scanner_state_t*)malloc(sizeof(ble_scanner_state_t));
    if (!state) return false;
    
    // Initialize state
    memset(state, 0, sizeof(ble_scanner_state_t));
    state->ui_state = STATE_MENU;
    state->selected_device = 0;
    state->scan_time_seconds = 10;
    state->show_unnamed = true;
    state->active_scan = true;
    
    ctx->private_data = state;
    ctx->private_data_size = sizeof(ble_scanner_state_t);
    
    // Initialize BLE
    BLEDevice::init("Izod Mini Scanner");
    pBLEScan = BLEDevice::getScan();
    pScanCallback = new BLEScanCallback(state);
    pBLEScan->setAdvertisedDeviceCallbacks(pScanCallback);
    pBLEScan->setActiveScan(state->active_scan);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    
    const plugin_hal_t* hal = plugin_get_hal();
    if (hal && hal->system) {
        hal->system->log("INFO", "BLE Scanner initialized");
    }
    
    return true;
}

// Plugin main loop
static void ble_scanner_run(plugin_context_t* ctx) {
    if (!ctx || !ctx->private_data) return;
    
    ble_scanner_state_t* state = (ble_scanner_state_t*)ctx->private_data;
    const plugin_hal_t* hal = plugin_get_hal();
    if (!hal) return;
    
    uint32_t now = hal->system->get_time_ms();
    
    // Handle scan completion
    if (state->scanning && pBLEScan && pBLEScan->isScanning() == false) {
        state->scanning = false;
        state->ui_state = STATE_DEVICE_LIST;
        state->total_scans++;
        state->selected_device = 0;
    }
    
    // Update display every 100ms
    static uint32_t last_display_update = 0;
    if (now - last_display_update > 100) {
        hal->display->clear(PLUGIN_COLOR_BLACK);
        
        switch (state->ui_state) {
            case STATE_MENU:
                draw_menu(state, hal);
                break;
            case STATE_SCANNING:
                hal->display->text(10, 10, "BLE Scanner", PLUGIN_COLOR_WHITE, 2);
                hal->display->text(10, 40, "Scanning for devices...", PLUGIN_COLOR_YELLOW, 1);
                
                // Progress bar
                uint32_t elapsed = (now - state->last_scan_time) / 1000;
                int progress = (elapsed * 100) / state->scan_time_seconds;
                progress = constrain(progress, 0, 100);
                
                hal->display->rect(10, 60, 200, 10, PLUGIN_COLOR_WHITE);
                hal->display->fill_rect(12, 62, (progress * 196) / 100, 6, PLUGIN_COLOR_GREEN);
                
                char scan_info[50];
                snprintf(scan_info, sizeof(scan_info), "Time: %lu/%d s  Found: %d", 
                         elapsed, state->scan_time_seconds, state->device_count);
                hal->display->text(10, 80, scan_info, PLUGIN_COLOR_GRAY, 1);
                
                char packet_info[30];
                snprintf(packet_info, sizeof(packet_info), "Packets: %lu", state->packets_received);
                hal->display->text(10, 100, packet_info, PLUGIN_COLOR_GRAY, 1);
                break;
            case STATE_DEVICE_LIST:
                draw_device_list(state, hal);
                break;
            case STATE_DEVICE_DETAILS:
                draw_device_details(state, hal);
                break;
            case STATE_SERVICES:
                draw_services(state, hal);
                break;
            case STATE_SETTINGS:
                draw_settings(state, hal);
                break;
        }
        
        hal->display->update();
        last_display_update = now;
    }
    
    hal->system->delay_ms(10);
}

// Plugin cleanup
static void ble_scanner_cleanup(plugin_context_t* ctx) {
    if (!ctx || !ctx->private_data) return;
    
    ble_scanner_state_t* state = (ble_scanner_state_t*)ctx->private_data;
    
    // Stop scanning
    stop_ble_scan(state);
    
    // Cleanup BLE
    if (pScanCallback) {
        delete pScanCallback;
        pScanCallback = nullptr;
    }
    
    BLEDevice::deinit(false);
    
    const plugin_hal_t* hal = plugin_get_hal();
    if (hal && hal->system) {
        hal->system->log("INFO", "BLE Scanner cleanup");
    }
    
    free(ctx->private_data);
    ctx->private_data = nullptr;
    ctx->private_data_size = 0;
}

// Plugin event handler
static bool ble_scanner_event_handler(plugin_context_t* ctx, uint32_t event, void* data) {
    if (!ctx || !ctx->private_data) return false;
    
    ble_scanner_state_t* state = (ble_scanner_state_t*)ctx->private_data;
    const plugin_hal_t* hal = plugin_get_hal();
    
    if (event == PLUGIN_EVENT_BUTTON_PRESS) {
        plugin_button_event_t* btn_event = (plugin_button_event_t*)data;
        if (!btn_event) return false;
        
        switch (btn_event->button_id) {
            case PLUGIN_BUTTON_MENU:
                return false; // Exit plugin
                
            case PLUGIN_BUTTON_SELECT:
                switch (state->ui_state) {
                    case STATE_MENU:
                        switch (state->selected_device) {
                            case 0: // Start Scan
                                start_ble_scan(state);
                                break;
                            case 1: // View Devices
                                if (state->device_count > 0) {
                                    state->ui_state = STATE_DEVICE_LIST;
                                    state->selected_device = 0;
                                }
                                break;
                            case 2: // Settings
                                state->ui_state = STATE_SETTINGS;
                                state->selected_device = 0;
                                break;
                            case 3: // Save Results
                                save_scan_results(state, hal);
                                break;
                        }
                        break;
                        
                    case STATE_DEVICE_LIST:
                        if (state->device_count > 0) {
                            state->ui_state = STATE_DEVICE_DETAILS;
                        }
                        break;
                        
                    case STATE_DEVICE_DETAILS:
                        if (state->device_list[state->selected_device].service_count > 0) {
                            state->ui_state = STATE_SERVICES;
                        }
                        break;
                        
                    case STATE_SETTINGS:
                        // Toggle settings
                        switch (state->selected_device) {
                            case 0: // Scan time
                                state->scan_time_seconds = (state->scan_time_seconds == 10) ? 30 : 
                                                          (state->scan_time_seconds == 30) ? 60 : 10;
                                break;
                            case 1: // Show unnamed
                                state->show_unnamed = !state->show_unnamed;
                                break;
                            case 2: // Active scan
                                state->active_scan = !state->active_scan;
                                if (pBLEScan) {
                                    pBLEScan->setActiveScan(state->active_scan);
                                }
                                break;
                        }
                        break;
                }
                break;
                
            case PLUGIN_BUTTON_PREVIOUS:
                switch (state->ui_state) {
                    case STATE_MENU:
                        state->selected_device = (state->selected_device - 1 + 4) % 4;
                        break;
                    case STATE_DEVICE_LIST:
                        if (state->device_count > 0) {
                            state->selected_device = (state->selected_device - 1 + state->device_count) % state->device_count;
                        }
                        break;
                    case STATE_SETTINGS:
                        state->selected_device = (state->selected_device - 1 + 3) % 3;
                        break;
                }
                break;
                
            case PLUGIN_BUTTON_NEXT:
                switch (state->ui_state) {
                    case STATE_MENU:
                        state->selected_device = (state->selected_device + 1) % 4;
                        break;
                    case STATE_DEVICE_LIST:
                        if (state->device_count > 0) {
                            state->selected_device = (state->selected_device + 1) % state->device_count;
                        }
                        break;
                    case STATE_SETTINGS:
                        state->selected_device = (state->selected_device + 1) % 3;
                        break;
                }
                break;
                
            case PLUGIN_BUTTON_PLAY:
                // Back/stop
                switch (state->ui_state) {
                    case STATE_SCANNING:
                        stop_ble_scan(state);
                        state->ui_state = STATE_MENU;
                        break;
                    case STATE_DEVICE_LIST:
                    case STATE_SETTINGS:
                        state->ui_state = STATE_MENU;
                        state->selected_device = 0;
                        break;
                    case STATE_DEVICE_DETAILS:
                        state->ui_state = STATE_DEVICE_LIST;
                        break;
                    case STATE_SERVICES:
                        state->ui_state = STATE_DEVICE_DETAILS;
                        break;
                }
                break;
        }
        
        return true;
    }
    
    return false;
}

// Internal functions
static void start_ble_scan(ble_scanner_state_t* state) {
    if (!pBLEScan) return;
    
    state->scanning = true;
    state->ui_state = STATE_SCANNING;
    state->device_count = 0;
    state->packets_received = 0;
    state->last_scan_time = millis();
    
    // Clear device list
    memset(state->device_list, 0, sizeof(state->device_list));
    
    // Start scan
    pBLEScan->start(state->scan_time_seconds, false);
}

static void stop_ble_scan(ble_scanner_state_t* state) {
    if (pBLEScan && pBLEScan->isScanning()) {
        pBLEScan->stop();
    }
    state->scanning = false;
}

static void save_scan_results(ble_scanner_state_t* state, const plugin_hal_t* hal) {
    if (!hal->storage) return;
    
    // Create filename with timestamp
    char filename[64];
    snprintf(filename, sizeof(filename), "/Pentest/ble/scan_%lu.txt", millis());
    
    void* file = hal->storage->open(filename, "w");
    if (!file) return;
    
    // Write header
    char header[128];
    snprintf(header, sizeof(header), "BLE Scan Results - %d devices found\n", state->device_count);
    hal->storage->write(file, header, strlen(header));
    
    const char* column_header = "Name                          Address           RSSI  Type\n";
    hal->storage->write(file, column_header, strlen(column_header));
    
    const char* separator = "================================================================\n";
    hal->storage->write(file, separator, strlen(separator));
    
    // Write device data
    for (int i = 0; i < state->device_count; i++) {
        char line[128];
        snprintf(line, sizeof(line), "%-30s %s  %4d  %s\n",
                 state->device_list[i].name,
                 state->device_list[i].address,
                 state->device_list[i].rssi,
                 get_device_type_string(state->device_list[i].appearance));
        
        hal->storage->write(file, line, strlen(line));
    }
    
    hal->storage->close(file);
    
    if (hal->system) {
        hal->system->log("INFO", "BLE scan results saved");
    }
}

static void draw_menu(ble_scanner_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "BLE Scanner", PLUGIN_COLOR_WHITE, 2);
    
    const char* menu_items[] = {
        "Start Scan",
        "View Devices",
        "Settings",
        "Save Results"
    };
    
    for (int i = 0; i < 4; i++) {
        uint16_t color = (i == state->selected_device) ? PLUGIN_COLOR_GREEN : PLUGIN_COLOR_WHITE;
        hal->display->text(20, 50 + i * 20, menu_items[i], color, 1);
        
        if (i == state->selected_device) {
            hal->display->text(10, 50 + i * 20, ">", PLUGIN_COLOR_GREEN, 1);
        }
    }
    
    // Statistics
    char stats[60];
    snprintf(stats, sizeof(stats), "Devices: %lu  Scans: %lu  Packets: %lu", 
             state->unique_devices, state->total_scans, state->packets_received);
    hal->display->text(10, 160, stats, PLUGIN_COLOR_GRAY, 1);
    
    // Instructions
    hal->display->text(10, 220, "Select: Choose option", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 240, "Menu: Exit", PLUGIN_COLOR_GRAY, 1);
}

static void draw_device_list(ble_scanner_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "BLE Devices", PLUGIN_COLOR_WHITE, 2);
    
    if (state->device_count == 0) {
        hal->display->text(10, 50, "No devices found", PLUGIN_COLOR_RED, 1);
        hal->display->text(10, 70, "Press Play to go back", PLUGIN_COLOR_GRAY, 1);
        return;
    }
    
    // Show device list (max 9 visible)
    int start_idx = max(0, state->selected_device - 4);
    int end_idx = min(state->device_count, start_idx + 9);
    
    for (int i = start_idx; i < end_idx; i++) {
        int y = 35 + (i - start_idx) * 18;
        uint16_t color = (i == state->selected_device) ? PLUGIN_COLOR_GREEN : PLUGIN_COLOR_WHITE;
        
        if (i == state->selected_device) {
            hal->display->text(5, y, ">", PLUGIN_COLOR_GREEN, 1);
        }
        
        // Device name (truncated)
        char display_name[16];
        strncpy(display_name, state->device_list[i].name, 15);
        display_name[15] = '\0';
        hal->display->text(15, y, display_name, color, 1);
        
        // RSSI
        char rssi_str[6];
        snprintf(rssi_str, sizeof(rssi_str), "%4d", state->device_list[i].rssi);
        hal->display->text(140, y, rssi_str, color, 1);
        
        // Connectable indicator
        if (state->device_list[i].connectable) {
            hal->display->text(180, y, "C", PLUGIN_COLOR_BLUE, 1);
        }
        
        // Address type
        const char* addr_type = (state->device_list[i].addr_type == 0) ? "P" : "R";
        hal->display->text(200, y, addr_type, color, 1);
        
        // Service indicator
        if (state->device_list[i].service_count > 0) {
            hal->display->text(220, y, "S", PLUGIN_COLOR_YELLOW, 1);
        }
    }
    
    // Scroll indicator
    if (state->device_count > 9) {
        char scroll_info[20];
        snprintf(scroll_info, sizeof(scroll_info), "%d/%d", state->selected_device + 1, state->device_count);
        hal->display->text(200, 10, scroll_info, PLUGIN_COLOR_GRAY, 1);
    }
    
    // Legend
    hal->display->text(10, 220, "C=Connectable R=Random P=Public", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 240, "S=Services  Select: Details", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 260, "Play: Back", PLUGIN_COLOR_GRAY, 1);
}

static void draw_device_details(ble_scanner_state_t* state, const plugin_hal_t* hal) {
    if (state->selected_device >= state->device_count) return;
    
    auto& device = state->device_list[state->selected_device];
    
    hal->display->text(10, 10, "Device Details", PLUGIN_COLOR_WHITE, 2);
    
    // Name
    hal->display->text(10, 40, "Name:", PLUGIN_COLOR_WHITE, 1);
    hal->display->text(60, 40, device.name, PLUGIN_COLOR_YELLOW, 1);
    
    // Address
    hal->display->text(10, 60, "Address:", PLUGIN_COLOR_WHITE, 1);
    hal->display->text(80, 60, device.address, PLUGIN_COLOR_CYAN, 1);
    
    // RSSI and distance estimate
    char rssi_info[40];
    float distance = pow(10, (-30 - device.rssi) / 20.0);
    snprintf(rssi_info, sizeof(rssi_info), "RSSI: %d dBm (~%.1fm)", device.rssi, distance);
    hal->display->text(10, 80, rssi_info, PLUGIN_COLOR_WHITE, 1);
    
    // Address type and connectivity
    const char* addr_type_str = (device.addr_type == 0) ? "Public" : "Random";
    const char* conn_str = device.connectable ? "Yes" : "No";
    char type_info[40];
    snprintf(type_info, sizeof(type_info), "Type: %s  Connectable: %s", addr_type_str, conn_str);
    hal->display->text(10, 100, type_info, PLUGIN_COLOR_WHITE, 1);
    
    // Device type (from appearance)
    if (device.appearance > 0) {
        hal->display->text(10, 120, "Device Type:", PLUGIN_COLOR_WHITE, 1);
        hal->display->text(100, 120, get_device_type_string(device.appearance), PLUGIN_COLOR_GREEN, 1);
    }
    
    // Manufacturer
    if (device.manufacturer_id > 0) {
        char mfg_info[50];
        snprintf(mfg_info, sizeof(mfg_info), "Manufacturer: %s (0x%04X)", 
                 get_manufacturer_name(device.manufacturer_id), device.manufacturer_id);
        hal->display->text(10, 140, mfg_info, PLUGIN_COLOR_WHITE, 1);
    }
    
    // Services
    if (device.service_count > 0) {
        hal->display->text(10, 160, "Services:", PLUGIN_COLOR_WHITE, 1);
        char service_info[20];
        snprintf(service_info, sizeof(service_info), "%d found", device.service_count);
        hal->display->text(80, 160, service_info, PLUGIN_COLOR_YELLOW, 1);
        hal->display->text(10, 180, "Select: View services", PLUGIN_COLOR_GRAY, 1);
    }
    
    // Timestamps
    uint32_t age = (millis() - device.last_seen) / 1000;
    char time_info[30];
    snprintf(time_info, sizeof(time_info), "Last seen: %lu seconds ago", age);
    hal->display->text(10, 220, time_info, PLUGIN_COLOR_GRAY, 1);
    
    hal->display->text(10, 280, "Play: Back", PLUGIN_COLOR_GRAY, 1);
}

static void draw_services(ble_scanner_state_t* state, const plugin_hal_t* hal) {
    if (state->selected_device >= state->device_count) return;
    
    auto& device = state->device_list[state->selected_device];
    
    hal->display->text(10, 10, "Services", PLUGIN_COLOR_WHITE, 2);
    
    if (device.service_count == 0) {
        hal->display->text(10, 50, "No services found", PLUGIN_COLOR_RED, 1);
    } else {
        for (int i = 0; i < device.service_count && i < 5; i++) {
            int y = 40 + i * 20;
            hal->display->text(10, y, "UUID:", PLUGIN_COLOR_WHITE, 1);
            hal->display->text(50, y, device.services[i], PLUGIN_COLOR_CYAN, 1);
        }
    }
    
    hal->display->text(10, 280, "Play: Back", PLUGIN_COLOR_GRAY, 1);
}

static void draw_settings(ble_scanner_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "BLE Settings", PLUGIN_COLOR_WHITE, 2);
    
    const char* settings[] = {
        "Scan Duration",
        "Show Unnamed",
        "Active Scan"
    };
    
    for (int i = 0; i < 3; i++) {
        uint16_t color = (i == state->selected_device) ? PLUGIN_COLOR_GREEN : PLUGIN_COLOR_WHITE;
        hal->display->text(20, 50 + i * 25, settings[i], color, 1);
        
        if (i == state->selected_device) {
            hal->display->text(10, 50 + i * 25, ">", PLUGIN_COLOR_GREEN, 1);
        }
        
        // Setting values
        char value[20];
        switch (i) {
            case 0:
                snprintf(value, sizeof(value), "%d sec", state->scan_time_seconds);
                break;
            case 1:
                strcpy(value, state->show_unnamed ? "ON" : "OFF");
                break;
            case 2:
                strcpy(value, state->active_scan ? "ON" : "OFF");
                break;
        }
        hal->display->text(150, 50 + i * 25, value, color, 1);
    }
    
    hal->display->text(10, 200, "Select: Toggle", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 220, "Play: Back", PLUGIN_COLOR_GRAY, 1);
}

// Helper functions
static const char* get_device_type_string(uint16_t appearance) {
    switch (appearance) {
        case 64: return "Phone";
        case 128: return "Computer";
        case 192: return "Watch";
        case 320: return "Display";
        case 384: return "Remote Control";
        case 448: return "Eye-glasses";
        case 512: return "Tag";
        case 576: return "Keyring";
        case 640: return "Media Player";
        case 704: return "Barcode Scanner";
        case 768: return "Thermometer";
        case 832: return "Heart Rate Sensor";
        case 896: return "Blood Pressure";
        case 960: return "HID";
        case 1024: return "Glucose Meter";
        case 1088: return "Running/Walking Sensor";
        case 1152: return "Cycling";
        case 1216: return "Pulse Oximeter";
        case 1280: return "Weight Scale";
        case 1344: return "Outdoor Sports";
        default: return "Unknown";
    }
}

static const char* get_manufacturer_name(uint16_t id) {
    switch (id) {
        case 0x004C: return "Apple";
        case 0x0006: return "Microsoft";
        case 0x00E0: return "Google";
        case 0x0075: return "Samsung";
        case 0x0087: return "Garmin";
        case 0x0157: return "Fitbit";
        case 0x006D: return "Polar";
        case 0x0059: return "Nordic Semi";
        case 0x0131: return "Cypress";
        case 0x0001: return "Ericsson";
        default: return "Unknown";
    }
}

// Plugin entry point
extern "C" {
    const plugin_manifest_t* plugin_get_manifest(void) {
        ble_scanner_manifest.event_handler = ble_scanner_event_handler;
        ble_scanner_manifest.compatibility_flags = PLUGIN_COMPAT_TFT_DISPLAY | PLUGIN_COMPAT_PHYSICAL_BTNS | PLUGIN_COMPAT_BLUETOOTH;
        ble_scanner_manifest.memory_required = 14336;
        
        strncpy((char*)ble_scanner_manifest.description,
                "Bluetooth Low Energy device scanner with detailed device analysis",
                PLUGIN_DESC_MAX_LENGTH - 1);
        
        return &ble_scanner_manifest;
    }
}

