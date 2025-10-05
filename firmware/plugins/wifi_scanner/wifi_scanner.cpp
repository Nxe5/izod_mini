/*
 * WiFi Scanner Plugin
 * Comprehensive WiFi network scanner and analyzer
 */

#include "plugin_api.h"
#include <Arduino.h>
#include <WiFi.h>

// Plugin state
typedef struct {
    bool scanning;
    bool continuous_scan;
    int selected_ap;
    int ap_count;
    uint32_t last_scan_time;
    uint32_t last_update;
    uint8_t sort_mode; // 0=RSSI, 1=Channel, 2=SSID
    
    // AP list with extended info
    struct {
        char ssid[33];
        uint8_t bssid[6];
        int32_t rssi;
        uint8_t channel;
        wifi_auth_mode_t authmode;
        uint32_t first_seen;
        uint32_t last_seen;
        uint16_t beacon_count;
        bool hidden;
    } ap_list[50];
    
    // UI state
    enum {
        STATE_MENU,
        STATE_SCANNING,
        STATE_AP_LIST,
        STATE_AP_DETAILS,
        STATE_CHANNEL_GRAPH,
        STATE_SETTINGS
    } ui_state;
    
    // Settings
    bool show_hidden;
    bool auto_refresh;
    uint16_t scan_interval_ms;
    
    // Statistics
    uint32_t total_scans;
    uint32_t unique_networks;
    
} wifi_scanner_state_t;

// Function prototypes
static bool wifi_scanner_init(plugin_context_t* ctx);
static void wifi_scanner_run(plugin_context_t* ctx);
static void wifi_scanner_cleanup(plugin_context_t* ctx);
static bool wifi_scanner_event_handler(plugin_context_t* ctx, uint32_t event, void* data);

// Internal functions
static void start_wifi_scan(wifi_scanner_state_t* state);
static void process_scan_results(wifi_scanner_state_t* state);
static void sort_ap_list(wifi_scanner_state_t* state);
static void save_scan_results(wifi_scanner_state_t* state, const plugin_hal_t* hal);
static void draw_menu(wifi_scanner_state_t* state, const plugin_hal_t* hal);
static void draw_ap_list(wifi_scanner_state_t* state, const plugin_hal_t* hal);
static void draw_ap_details(wifi_scanner_state_t* state, const plugin_hal_t* hal);
static void draw_channel_graph(wifi_scanner_state_t* state, const plugin_hal_t* hal);
static void draw_settings(wifi_scanner_state_t* state, const plugin_hal_t* hal);
static const char* get_auth_mode_string(wifi_auth_mode_t authmode);
static const char* get_rssi_bar(int32_t rssi);

// Plugin manifest
static plugin_manifest_t wifi_scanner_manifest = PLUGIN_MANIFEST(
    "WiFi Scanner",
    "1.0.0",
    "Izod Mini Team",
    PLUGIN_CATEGORY_PENTEST,
    wifi_scanner_init,
    wifi_scanner_run,
    wifi_scanner_cleanup
);

// Plugin initialization
static bool wifi_scanner_init(plugin_context_t* ctx) {
    if (!ctx) return false;
    
    // Allocate plugin state
    wifi_scanner_state_t* state = (wifi_scanner_state_t*)malloc(sizeof(wifi_scanner_state_t));
    if (!state) return false;
    
    // Initialize state
    memset(state, 0, sizeof(wifi_scanner_state_t));
    state->ui_state = STATE_MENU;
    state->selected_ap = 0;
    state->sort_mode = 0; // Sort by RSSI
    state->show_hidden = true;
    state->auto_refresh = false;
    state->scan_interval_ms = 5000; // 5 seconds
    
    ctx->private_data = state;
    ctx->private_data_size = sizeof(wifi_scanner_state_t);
    
    // Initialize WiFi
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    const plugin_hal_t* hal = plugin_get_hal();
    if (hal && hal->system) {
        hal->system->log("INFO", "WiFi Scanner initialized");
    }
    
    return true;
}

// Plugin main loop
static void wifi_scanner_run(plugin_context_t* ctx) {
    if (!ctx || !ctx->private_data) return;
    
    wifi_scanner_state_t* state = (wifi_scanner_state_t*)ctx->private_data;
    const plugin_hal_t* hal = plugin_get_hal();
    if (!hal) return;
    
    uint32_t now = hal->system->get_time_ms();
    
    // Handle scanning
    if (state->scanning && (now - state->last_update > 3000)) {
        process_scan_results(state);
        state->scanning = false;
        state->ui_state = STATE_AP_LIST;
        state->total_scans++;
    }
    
    // Auto-refresh scanning
    if (state->auto_refresh && !state->scanning && 
        (now - state->last_scan_time > state->scan_interval_ms)) {
        start_wifi_scan(state);
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
                hal->display->text(10, 10, "WiFi Scanner", PLUGIN_COLOR_WHITE, 2);
                hal->display->text(10, 40, "Scanning networks...", PLUGIN_COLOR_YELLOW, 1);
                
                // Progress indicator
                int progress = ((now - state->last_update) * 100) / 3000;
                hal->display->rect(10, 60, 200, 10, PLUGIN_COLOR_WHITE);
                hal->display->fill_rect(12, 62, (progress * 196) / 100, 6, PLUGIN_COLOR_GREEN);
                
                char scan_info[30];
                snprintf(scan_info, sizeof(scan_info), "Scan #%lu", state->total_scans + 1);
                hal->display->text(10, 80, scan_info, PLUGIN_COLOR_GRAY, 1);
                break;
            case STATE_AP_LIST:
                draw_ap_list(state, hal);
                break;
            case STATE_AP_DETAILS:
                draw_ap_details(state, hal);
                break;
            case STATE_CHANNEL_GRAPH:
                draw_channel_graph(state, hal);
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
static void wifi_scanner_cleanup(plugin_context_t* ctx) {
    if (!ctx || !ctx->private_data) return;
    
    wifi_scanner_state_t* state = (wifi_scanner_state_t*)ctx->private_data;
    
    // Stop scanning
    WiFi.scanDelete();
    
    const plugin_hal_t* hal = plugin_get_hal();
    if (hal && hal->system) {
        hal->system->log("INFO", "WiFi Scanner cleanup");
    }
    
    free(ctx->private_data);
    ctx->private_data = nullptr;
    ctx->private_data_size = 0;
}

// Plugin event handler
static bool wifi_scanner_event_handler(plugin_context_t* ctx, uint32_t event, void* data) {
    if (!ctx || !ctx->private_data) return false;
    
    wifi_scanner_state_t* state = (wifi_scanner_state_t*)ctx->private_data;
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
                        switch (state->selected_ap) {
                            case 0: // Quick Scan
                                start_wifi_scan(state);
                                break;
                            case 1: // View Results
                                if (state->ap_count > 0) {
                                    state->ui_state = STATE_AP_LIST;
                                    state->selected_ap = 0;
                                }
                                break;
                            case 2: // Channel Graph
                                state->ui_state = STATE_CHANNEL_GRAPH;
                                break;
                            case 3: // Settings
                                state->ui_state = STATE_SETTINGS;
                                state->selected_ap = 0;
                                break;
                            case 4: // Save Results
                                save_scan_results(state, hal);
                                break;
                        }
                        break;
                        
                    case STATE_AP_LIST:
                        if (state->ap_count > 0) {
                            state->ui_state = STATE_AP_DETAILS;
                        }
                        break;
                        
                    case STATE_SETTINGS:
                        // Toggle settings
                        switch (state->selected_ap) {
                            case 0: // Sort mode
                                state->sort_mode = (state->sort_mode + 1) % 3;
                                sort_ap_list(state);
                                break;
                            case 1: // Show hidden
                                state->show_hidden = !state->show_hidden;
                                break;
                            case 2: // Auto refresh
                                state->auto_refresh = !state->auto_refresh;
                                break;
                        }
                        break;
                }
                break;
                
            case PLUGIN_BUTTON_PREVIOUS:
                switch (state->ui_state) {
                    case STATE_MENU:
                        state->selected_ap = (state->selected_ap - 1 + 5) % 5;
                        break;
                    case STATE_AP_LIST:
                        if (state->ap_count > 0) {
                            state->selected_ap = (state->selected_ap - 1 + state->ap_count) % state->ap_count;
                        }
                        break;
                    case STATE_SETTINGS:
                        state->selected_ap = (state->selected_ap - 1 + 3) % 3;
                        break;
                }
                break;
                
            case PLUGIN_BUTTON_NEXT:
                switch (state->ui_state) {
                    case STATE_MENU:
                        state->selected_ap = (state->selected_ap + 1) % 5;
                        break;
                    case STATE_AP_LIST:
                        if (state->ap_count > 0) {
                            state->selected_ap = (state->selected_ap + 1) % state->ap_count;
                        }
                        break;
                    case STATE_SETTINGS:
                        state->selected_ap = (state->selected_ap + 1) % 3;
                        break;
                }
                break;
                
            case PLUGIN_BUTTON_PLAY:
                // Back/refresh
                switch (state->ui_state) {
                    case STATE_AP_LIST:
                    case STATE_CHANNEL_GRAPH:
                    case STATE_SETTINGS:
                        state->ui_state = STATE_MENU;
                        state->selected_ap = 0;
                        break;
                    case STATE_AP_DETAILS:
                        state->ui_state = STATE_AP_LIST;
                        break;
                    case STATE_MENU:
                        // Refresh scan
                        start_wifi_scan(state);
                        break;
                }
                break;
        }
        
        return true;
    }
    
    return false;
}

// Internal functions
static void start_wifi_scan(wifi_scanner_state_t* state) {
    state->scanning = true;
    state->ui_state = STATE_SCANNING;
    state->last_update = millis();
    state->last_scan_time = millis();
    
    WiFi.scanNetworks(true, state->show_hidden); // Async scan
}

static void process_scan_results(wifi_scanner_state_t* state) {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) {
        state->ap_count = 0;
        return;
    }
    
    uint32_t now = millis();
    state->ap_count = min(n, 50); // Limit to 50 APs
    
    for (int i = 0; i < state->ap_count; i++) {
        String ssid = WiFi.SSID(i);
        strncpy(state->ap_list[i].ssid, ssid.c_str(), 32);
        state->ap_list[i].ssid[32] = '\0';
        
        // Get BSSID
        uint8_t* bssid = WiFi.BSSID(i);
        if (bssid) {
            memcpy(state->ap_list[i].bssid, bssid, 6);
        }
        
        state->ap_list[i].rssi = WiFi.RSSI(i);
        state->ap_list[i].channel = WiFi.channel(i);
        state->ap_list[i].authmode = WiFi.encryptionType(i);
        state->ap_list[i].hidden = (ssid.length() == 0);
        state->ap_list[i].first_seen = now;
        state->ap_list[i].last_seen = now;
        state->ap_list[i].beacon_count = 1;
    }
    
    sort_ap_list(state);
    WiFi.scanDelete();
    state->selected_ap = 0;
    state->unique_networks = state->ap_count;
}

static void sort_ap_list(wifi_scanner_state_t* state) {
    // Simple bubble sort (good enough for small lists)
    for (int i = 0; i < state->ap_count - 1; i++) {
        for (int j = 0; j < state->ap_count - i - 1; j++) {
            bool swap = false;
            
            switch (state->sort_mode) {
                case 0: // RSSI (strongest first)
                    swap = state->ap_list[j].rssi < state->ap_list[j + 1].rssi;
                    break;
                case 1: // Channel
                    swap = state->ap_list[j].channel > state->ap_list[j + 1].channel;
                    break;
                case 2: // SSID
                    swap = strcmp(state->ap_list[j].ssid, state->ap_list[j + 1].ssid) > 0;
                    break;
            }
            
            if (swap) {
                // Swap elements
                auto temp = state->ap_list[j];
                state->ap_list[j] = state->ap_list[j + 1];
                state->ap_list[j + 1] = temp;
            }
        }
    }
}

static void save_scan_results(wifi_scanner_state_t* state, const plugin_hal_t* hal) {
    if (!hal->storage) return;
    
    // Create filename with timestamp
    char filename[64];
    snprintf(filename, sizeof(filename), "/Pentest/wifi/scan_%lu.txt", millis());
    
    void* file = hal->storage->open(filename, "w");
    if (!file) return;
    
    // Write header
    char header[128];
    snprintf(header, sizeof(header), "WiFi Scan Results - %lu networks found\n", state->ap_count);
    hal->storage->write(file, header, strlen(header));
    
    const char* column_header = "SSID                          BSSID             Ch  RSSI  Auth\n";
    hal->storage->write(file, column_header, strlen(column_header));
    
    const char* separator = "================================================================\n";
    hal->storage->write(file, separator, strlen(separator));
    
    // Write AP data
    for (int i = 0; i < state->ap_count; i++) {
        char line[128];
        char bssid_str[18];
        snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 state->ap_list[i].bssid[0], state->ap_list[i].bssid[1],
                 state->ap_list[i].bssid[2], state->ap_list[i].bssid[3],
                 state->ap_list[i].bssid[4], state->ap_list[i].bssid[5]);
        
        snprintf(line, sizeof(line), "%-30s %s  %2d  %4d  %s\n",
                 state->ap_list[i].hidden ? "<Hidden>" : state->ap_list[i].ssid,
                 bssid_str,
                 state->ap_list[i].channel,
                 state->ap_list[i].rssi,
                 get_auth_mode_string(state->ap_list[i].authmode));
        
        hal->storage->write(file, line, strlen(line));
    }
    
    hal->storage->close(file);
    
    if (hal->system) {
        hal->system->log("INFO", "Scan results saved");
    }
}

static void draw_menu(wifi_scanner_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "WiFi Scanner", PLUGIN_COLOR_WHITE, 2);
    
    const char* menu_items[] = {
        "Quick Scan",
        "View Results",
        "Channel Graph", 
        "Settings",
        "Save Results"
    };
    
    for (int i = 0; i < 5; i++) {
        uint16_t color = (i == state->selected_ap) ? PLUGIN_COLOR_GREEN : PLUGIN_COLOR_WHITE;
        hal->display->text(20, 50 + i * 20, menu_items[i], color, 1);
        
        if (i == state->selected_ap) {
            hal->display->text(10, 50 + i * 20, ">", PLUGIN_COLOR_GREEN, 1);
        }
    }
    
    // Statistics
    char stats[50];
    snprintf(stats, sizeof(stats), "Networks: %lu  Scans: %lu", state->unique_networks, state->total_scans);
    hal->display->text(10, 180, stats, PLUGIN_COLOR_GRAY, 1);
    
    // Instructions
    hal->display->text(10, 220, "Select: Choose", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 240, "Play: Quick scan", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 260, "Menu: Exit", PLUGIN_COLOR_GRAY, 1);
}

static void draw_ap_list(wifi_scanner_state_t* state, const plugin_hal_t* hal) {
    const char* sort_names[] = {"RSSI", "Channel", "SSID"};
    char title[50];
    snprintf(title, sizeof(title), "Networks (by %s)", sort_names[state->sort_mode]);
    hal->display->text(10, 10, title, PLUGIN_COLOR_WHITE, 2);
    
    if (state->ap_count == 0) {
        hal->display->text(10, 50, "No networks found", PLUGIN_COLOR_RED, 1);
        hal->display->text(10, 70, "Press Play to go back", PLUGIN_COLOR_GRAY, 1);
        return;
    }
    
    // Show AP list (max 9 visible)
    int start_idx = max(0, state->selected_ap - 4);
    int end_idx = min(state->ap_count, start_idx + 9);
    
    for (int i = start_idx; i < end_idx; i++) {
        int y = 35 + (i - start_idx) * 18;
        uint16_t color = (i == state->selected_ap) ? PLUGIN_COLOR_GREEN : PLUGIN_COLOR_WHITE;
        
        if (i == state->selected_ap) {
            hal->display->text(5, y, ">", PLUGIN_COLOR_GREEN, 1);
        }
        
        // SSID (truncated)
        char display_ssid[16];
        if (state->ap_list[i].hidden) {
            strcpy(display_ssid, "<Hidden>");
        } else {
            strncpy(display_ssid, state->ap_list[i].ssid, 15);
            display_ssid[15] = '\0';
        }
        hal->display->text(15, y, display_ssid, color, 1);
        
        // Channel
        char ch_str[4];
        snprintf(ch_str, sizeof(ch_str), "%2d", state->ap_list[i].channel);
        hal->display->text(140, y, ch_str, color, 1);
        
        // RSSI bar
        hal->display->text(160, y, get_rssi_bar(state->ap_list[i].rssi), color, 1);
        
        // RSSI value
        char rssi_str[6];
        snprintf(rssi_str, sizeof(rssi_str), "%4d", state->ap_list[i].rssi);
        hal->display->text(190, y, rssi_str, color, 1);
        
        // Security
        if (state->ap_list[i].authmode != WIFI_AUTH_OPEN) {
            hal->display->text(225, y, "🔒", color, 1);
        }
    }
    
    // Scroll indicator
    if (state->ap_count > 9) {
        char scroll_info[20];
        snprintf(scroll_info, sizeof(scroll_info), "%d/%d", state->selected_ap + 1, state->ap_count);
        hal->display->text(200, 10, scroll_info, PLUGIN_COLOR_GRAY, 1);
    }
    
    // Instructions
    hal->display->text(10, 280, "Select: Details  Play: Back", PLUGIN_COLOR_GRAY, 1);
}

static void draw_ap_details(wifi_scanner_state_t* state, const plugin_hal_t* hal) {
    if (state->selected_ap >= state->ap_count) return;
    
    auto& ap = state->ap_list[state->selected_ap];
    
    hal->display->text(10, 10, "Network Details", PLUGIN_COLOR_WHITE, 2);
    
    // SSID
    hal->display->text(10, 40, "SSID:", PLUGIN_COLOR_WHITE, 1);
    const char* ssid_display = ap.hidden ? "<Hidden Network>" : ap.ssid;
    hal->display->text(60, 40, ssid_display, PLUGIN_COLOR_YELLOW, 1);
    
    // BSSID
    hal->display->text(10, 60, "BSSID:", PLUGIN_COLOR_WHITE, 1);
    char bssid_str[18];
    snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3], ap.bssid[4], ap.bssid[5]);
    hal->display->text(60, 60, bssid_str, PLUGIN_COLOR_CYAN, 1);
    
    // Channel and RSSI
    char ch_rssi[30];
    snprintf(ch_rssi, sizeof(ch_rssi), "Channel: %d    RSSI: %d dBm", ap.channel, ap.rssi);
    hal->display->text(10, 80, ch_rssi, PLUGIN_COLOR_WHITE, 1);
    
    // Security
    hal->display->text(10, 100, "Security:", PLUGIN_COLOR_WHITE, 1);
    hal->display->text(80, 100, get_auth_mode_string(ap.authmode), PLUGIN_COLOR_GREEN, 1);
    
    // Signal strength bar
    hal->display->text(10, 120, "Signal:", PLUGIN_COLOR_WHITE, 1);
    int signal_width = map(ap.rssi, -100, -30, 0, 150);
    signal_width = constrain(signal_width, 0, 150);
    
    hal->display->rect(70, 120, 152, 12, PLUGIN_COLOR_WHITE);
    if (signal_width > 0) {
        uint16_t bar_color = (ap.rssi > -50) ? PLUGIN_COLOR_GREEN : 
                            (ap.rssi > -70) ? PLUGIN_COLOR_YELLOW : PLUGIN_COLOR_RED;
        hal->display->fill_rect(71, 121, signal_width, 10, bar_color);
    }
    
    // Estimated distance (rough calculation)
    float distance = pow(10, (-30 - ap.rssi) / 20.0);
    char dist_str[30];
    if (distance < 1.0) {
        snprintf(dist_str, sizeof(dist_str), "Est. distance: %.1f m", distance);
    } else {
        snprintf(dist_str, sizeof(dist_str), "Est. distance: %.0f m", distance);
    }
    hal->display->text(10, 150, dist_str, PLUGIN_COLOR_GRAY, 1);
    
    // Frequency
    int freq = (ap.channel <= 14) ? (2412 + (ap.channel - 1) * 5) : (5000 + ap.channel * 5);
    char freq_str[20];
    snprintf(freq_str, sizeof(freq_str), "Frequency: %d MHz", freq);
    hal->display->text(10, 170, freq_str, PLUGIN_COLOR_GRAY, 1);
    
    hal->display->text(10, 280, "Play: Back", PLUGIN_COLOR_GRAY, 1);
}

static void draw_channel_graph(wifi_scanner_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "Channel Usage", PLUGIN_COLOR_WHITE, 2);
    
    if (state->ap_count == 0) {
        hal->display->text(10, 50, "No data available", PLUGIN_COLOR_RED, 1);
        return;
    }
    
    // Count APs per channel
    int channel_count[15] = {0}; // Channels 1-14 + others
    for (int i = 0; i < state->ap_count; i++) {
        int ch = state->ap_list[i].channel;
        if (ch >= 1 && ch <= 14) {
            channel_count[ch]++;
        } else {
            channel_count[14]++; // "Other" category
        }
    }
    
    // Find max count for scaling
    int max_count = 0;
    for (int i = 1; i <= 14; i++) {
        if (channel_count[i] > max_count) {
            max_count = channel_count[i];
        }
    }
    
    if (max_count == 0) return;
    
    // Draw graph
    int graph_height = 150;
    int bar_width = 15;
    int start_x = 20;
    int start_y = 200;
    
    for (int i = 1; i <= 14; i++) {
        int bar_height = (channel_count[i] * graph_height) / max_count;
        int x = start_x + (i - 1) * bar_width;
        int y = start_y - bar_height;
        
        // Draw bar
        uint16_t color = PLUGIN_COLOR_BLUE;
        if (i == 1 || i == 6 || i == 11) color = PLUGIN_COLOR_GREEN; // Non-overlapping channels
        
        hal->display->fill_rect(x, y, bar_width - 2, bar_height, color);
        
        // Channel number
        char ch_str[3];
        snprintf(ch_str, sizeof(ch_str), "%d", i);
        hal->display->text(x + 2, start_y + 5, ch_str, PLUGIN_COLOR_WHITE, 1);
        
        // Count
        if (channel_count[i] > 0) {
            char count_str[3];
            snprintf(count_str, sizeof(count_str), "%d", channel_count[i]);
            hal->display->text(x + 2, y - 12, count_str, PLUGIN_COLOR_WHITE, 1);
        }
    }
    
    // Legend
    hal->display->text(10, 230, "Green: Non-overlapping (1,6,11)", PLUGIN_COLOR_GREEN, 1);
    hal->display->text(10, 250, "Blue: Other channels", PLUGIN_COLOR_BLUE, 1);
    
    hal->display->text(10, 280, "Play: Back", PLUGIN_COLOR_GRAY, 1);
}

static void draw_settings(wifi_scanner_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "Scanner Settings", PLUGIN_COLOR_WHITE, 2);
    
    const char* settings[] = {
        "Sort Mode",
        "Show Hidden",
        "Auto Refresh"
    };
    
    for (int i = 0; i < 3; i++) {
        uint16_t color = (i == state->selected_ap) ? PLUGIN_COLOR_GREEN : PLUGIN_COLOR_WHITE;
        hal->display->text(20, 50 + i * 25, settings[i], color, 1);
        
        if (i == state->selected_ap) {
            hal->display->text(10, 50 + i * 25, ">", PLUGIN_COLOR_GREEN, 1);
        }
        
        // Setting values
        char value[20];
        switch (i) {
            case 0: {
                const char* sort_names[] = {"RSSI", "Channel", "SSID"};
                strcpy(value, sort_names[state->sort_mode]);
                break;
            }
            case 1:
                strcpy(value, state->show_hidden ? "ON" : "OFF");
                break;
            case 2:
                strcpy(value, state->auto_refresh ? "ON" : "OFF");
                break;
        }
        hal->display->text(150, 50 + i * 25, value, color, 1);
    }
    
    hal->display->text(10, 200, "Select: Toggle", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 220, "Play: Back", PLUGIN_COLOR_GRAY, 1);
}

// Helper functions
static const char* get_auth_mode_string(wifi_auth_mode_t authmode) {
    switch (authmode) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Ent";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        default: return "Unknown";
    }
}

static const char* get_rssi_bar(int32_t rssi) {
    if (rssi > -30) return "████";
    else if (rssi > -50) return "███ ";
    else if (rssi > -70) return "██  ";
    else if (rssi > -90) return "█   ";
    else return "    ";
}

// Plugin entry point
extern "C" {
    const plugin_manifest_t* plugin_get_manifest(void) {
        wifi_scanner_manifest.event_handler = wifi_scanner_event_handler;
        wifi_scanner_manifest.compatibility_flags = PLUGIN_COMPAT_TFT_DISPLAY | PLUGIN_COMPAT_PHYSICAL_BTNS | PLUGIN_COMPAT_WIFI;
        wifi_scanner_manifest.memory_required = 12288;
        
        strncpy((char*)wifi_scanner_manifest.description,
                "Comprehensive WiFi network scanner and analyzer with detailed information",
                PLUGIN_DESC_MAX_LENGTH - 1);
        
        return &wifi_scanner_manifest;
    }
}

