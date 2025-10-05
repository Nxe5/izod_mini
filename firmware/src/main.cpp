#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <SD.h>

// New hardware configuration system
#include "hardware_config.h"
#include "touch_config.h"
#include "plugin_api.h"

// Legacy includes for compatibility
#include "app_state.h"
#include "ui_display.h"
#include "audio.h"
#include "version.h"
#include "audio_wav.h"
#include "audio_mp3.h"
#include "touch_wheel.h"

// Touch sensitivity management
extern bool touch_sensitivity_manager_init();
extern bool touch_calibration_init();
extern void touch_calibration_update();
extern void touch_sensitivity_manager_print_status();

// Plugin system
extern bool plugin_manager_init();
extern void plugin_manager_update();

// Display instance and mutex (using new hardware config)
Adafruit_ST7789 display(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);
SemaphoreHandle_t displayMutex = NULL;

// Status LED (if available)
#if STATUS_LED_PIN != -1
static const int STATUS_LED = STATUS_LED_PIN;
#else
static const int STATUS_LED = -1;
#endif

static void blinkStatusLED(bool state) {
    #if STATUS_LED_PIN != -1
    digitalWrite(STATUS_LED, state);
    #endif
}

// Optional UART on pins RX=38, TX=39
// Serial1 disabled to avoid conflict with I2S on GPIO38/39
static void initAuxUart() {}

// Tasks
TaskHandle_t displayTaskHandle = NULL;
TaskHandle_t heartbeatTaskHandle = NULL;
TaskHandle_t animationTaskHandle = NULL;
TaskHandle_t menuTaskHandle = NULL;
TaskHandle_t statusTaskHandle = NULL;

// Readiness flags
static volatile bool g_audioReady = false;
static volatile bool g_sdMounted = false;
static volatile int g_wheelDebugCounter = 6; // Start at 6, changes with wheel

// Wheel counter overlay - draws after UI refresh
static void drawWheelCounter() {
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        // Top-right corner overlay that won't be cleared by UI
        display.fillRect(200, 5, 35, 15, UI_COLOR_BG);
        display.drawRect(199, 4, 37, 17, UI_COLOR_FG); // border
        display.setTextColor(UI_COLOR_FG);
        display.setTextSize(1);
        display.setCursor(205, 8);
        display.printf("W:%d", g_wheelDebugCounter);
        Serial.printf("Wheel counter drawn: %d\n", g_wheelDebugCounter);
        xSemaphoreGive(displayMutex);
    } else {
        Serial.println("Wheel counter: failed to get display mutex");
    }
}

static void listSdFiles(const char* path = "/") {
    File root = SD.open(path);
    if (!root) { Serial.println("SD: failed to open root"); return; }
    if (!root.isDirectory()) { Serial.println("SD: root is not dir"); root.close(); return; }
    File f;
    while ((f = root.openNextFile())) {
        Serial.print(f.isDirectory() ? "[DIR] " : "      ");
        Serial.println(f.name());
        f.close();
    }
    root.close();
}

static bool ensureDir(const char* path) {
    if (SD.exists(path)) return true;
    return SD.mkdir(path);
}

static void initSdLayout() {
    // Base folders
    ensureDir("/Music");
    ensureDir("/Playlists");
    ensureDir("/Pentest");
    ensureDir("/Apps");
    ensureDir("/Firmware");
    ensureDir("/System");
    ensureDir("/System/thumbs");
    // Pentest subfolders (Flipper-like)
    ensureDir("/Pentest/wifi");
    ensureDir("/Pentest/ble");
    ensureDir("/Pentest/lf_rfid");
    ensureDir("/Pentest/nfc");
    ensureDir("/Pentest/ir");
    ensureDir("/Pentest/subghz");
    ensureDir("/Pentest/nrf24");
    ensureDir("/Pentest/badusb");
    ensureDir("/Pentest/scripts");
    ensureDir("/Pentest/captures");
    ensureDir("/Pentest/dumps");
    // Seed minimal files if desired
    if (!SD.exists("/System/library.json")) {
        File f = SD.open("/System/library.json", FILE_WRITE);
        if (f) { f.print("{\n  \"tracks\": []\n}\n"); f.close(); }
    }
}

static bool sdLayoutValid() {
    // Minimal validity: /System/library.json exists and /Music exists
    if (!SD.exists("/System")) return false;
    if (!SD.exists("/System/library.json")) return false;
    if (!SD.exists("/Music")) return false;
    return true;
}

static bool wipeDir(const String& path) {
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) { if (dir) dir.close(); return false; }
    File f;
    while ((f = dir.openNextFile())) {
        String child = String(path) + "/" + f.name();
        bool ok = true;
        if (f.isDirectory()) {
            f.close();
            ok = wipeDir(child) && SD.rmdir(child.c_str());
        } else {
            f.close();
            ok = SD.remove(child.c_str());
        }
        if (!ok) { dir.close(); return false; }
    }
    dir.close();
    return true;
}

static bool sdQuickFormat() {
    // WARNING: deletes all files/dirs in root, then recreates layout
    Serial.println("SD: QUICK FORMAT (deleting all files in /)");
    if (!wipeDir("/")) {
        Serial.println("SD: quick format failed during delete");
        return false;
    }
    initSdLayout();
    Serial.println("SD: layout recreated");
    return true;
}

// Display task
void displayTask(void *pvParameters) {
    SPI.begin(TFT_SCK_PIN, TFT_MISO_PIN, TFT_MOSI_PIN, TFT_CS_PIN);
    display.init(TFT_WIDTH, TFT_HEIGHT, SPI_MODE0);
    display.setRotation(TFT_ROTATION);
    display.setSPISpeed(TFT_SPI_FREQUENCY);
    display.fillScreen(UI_COLOR_BG);

    uiInit(&display, &displayMutex);

    // Splash screen on boot
    uiShowSplash("Ocho Labs", izod_firmware_name(), izod_firmware_version(), "Official build", UI_COLOR_HI);
    vTaskDelay(pdMS_TO_TICKS(2000));

    int counter = 0;
    uint32_t lastFooter = 0;

    while(1) {
        if (appConsumeRedrawRequest()) {
            if (appGetCurrentView() == UIView::VIEW_MENU) {
                if (appGetMenuLevel() == 0) uiDrawHome(); else uiDrawMusic();
    } else {
                uiDrawNowPlayingFull();
            }
        }
        if (millis() - lastFooter > 500) {
            drawWheelCounter(); // Draw wheel counter overlay
            lastFooter = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Heartbeat + timers
void heartbeatTask(void *pvParameters) {
    bool led_state = false;
    int uptime = 0;
    for(;;) {
        led_state = !led_state;
        digitalWrite(LED_BUILTIN, led_state);
        uptime++;
        if (appGetCurrentView() == UIView::VIEW_NOW_PLAYING) {
            appIncrementNowPlayingSecondsMod(180);
            uiUpdateNowPlayingProgress();
        }
        Serial.printf("Heartbeat: %d sec, tasks=%d\n", uptime, uxTaskGetNumberOfTasks());
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Animation bar (loading)
void animationTask(void *pvParameters) {
    int frame = 0;
    for(;;) {
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            const int bar_x = 10, bar_y = 235, bar_w = 220, bar_h = 15;
            display.drawRect(bar_x, bar_y, bar_w, bar_h, UI_COLOR_FG);
            int fill_w = (frame % 100) * (bar_w - 4) / 100;
            display.fillRect(bar_x + 2, bar_y + 2, fill_w, bar_h - 4, UI_COLOR_HI);
            xSemaphoreGive(displayMutex);
        }
        frame++;
        vTaskDelay(pdMS_TO_TICKS(80));
    }
}

static bool refreshSdMounted() {
    // Re-init and try opening root to detect card insertion/removal in SPI mode
    if (!SD.begin(SD_CS, SPI, 20000000)) {
        g_sdMounted = false;
        return false;
    }
    File root = SD.open("/");
    if (!root) { g_sdMounted = false; return false; }
    root.close();
    g_sdMounted = true;
    return true;
}

void statusTask(void *pvParameters) {
    for(;;) {
        bool audioOk = g_audioReady;
        bool sdOk = refreshSdMounted();
        bool playing = mp3IsPlaying() || wavIsPlaying() || audioIsPlaying();
        Serial.printf("Status: audio=%s, sd=%s, mp3=%s, playing=%s, vol=%d%%\n",
                      audioOk ? "OK" : "ERR",
                      sdOk ? "OK" : "NO",
                      mp3IsPlaying() ? "yes" : "no",
                      playing ? "yes" : "no",
                      audioGetVolume());
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// Menu/input task: u/d/s/b + audio p/+/-
void menuTask(void *pvParameters) {
    // Simple 5-button input on GPIOs 14,15,16,17,18 (INPUT_PULLUP)
    const int BTN_UP = 14, BTN_DOWN = 15, BTN_SEL = 16, BTN_BACK = 17, BTN_PLAY = 18;
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_SEL, INPUT_PULLUP);
    pinMode(BTN_BACK, INPUT_PULLUP);
    pinMode(BTN_PLAY, INPUT_PULLUP);
    uint8_t last = 0x1F; // all high (not pressed)
    uint32_t lastDebounceMs = 0;
    // Long-press tracking for Select
    static uint32_t selPressMs = 0;
    static bool selHeld = false;
    static bool selHoldTriggered = false;

    // Touch wheel init
    // Default mapping is 0..11; override here if your PCB pad order differs
    const uint8_t wheelOrder[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
    touchWheelSetElectrodeOrder(wheelOrder, 12);
    touchWheelSetInvert(false);
    bool wheelOk = touchWheelInit(MPR121_ADDR);
    Serial.printf("MPR121: %s on I2C SDA=%d SCL=%d addr=0x%02X\n", wheelOk?"OK":"not found", I2C_SDA, I2C_SCL, MPR121_ADDR);
    if (wheelOk) {
        // Increase sensitivity: lower thresholds (touch=4, release=2)
        touchWheelSetThresholds(4, 2);
        uint8_t t, r; touchWheelGetThresholds(&t, &r);
        Serial.printf("MPR121: thresholds touch=%u release=%u\n", t, r);
    }

    while(1) {
        // Button scan with debounce (active-low)
        uint8_t cur = 0;
        cur |= (digitalRead(BTN_UP)   == LOW) ? 0x01 : 0;
        cur |= (digitalRead(BTN_DOWN) == LOW) ? 0x02 : 0;
        cur |= (digitalRead(BTN_SEL)  == LOW) ? 0x04 : 0;
        cur |= (digitalRead(BTN_BACK) == LOW) ? 0x08 : 0;
        cur |= (digitalRead(BTN_PLAY) == LOW) ? 0x10 : 0;
        uint32_t nowMs = millis();
        // Edge handling
        if (cur != last && (nowMs - lastDebounceMs) > 25) {
            uint8_t pressed = (~last) & cur; // newly pressed bits
            uint8_t released = last & (~cur);
            if (pressed & 0x01) { // UP
                blinkNeo(0, 0, 40);
                if (appGetCurrentView() == UIView::VIEW_NOW_PLAYING) {
                    audioSetVolume(min(100, audioGetVolume() + 5));
                    char buf[32]; snprintf(buf, sizeof(buf), "Vol: %d%%", audioGetVolume()); uiToast(buf);
                } else {
                    int count = (appGetMenuLevel() == 0 ? 3 : 4);
                    int sel = appGetMenuSelected(); sel = (sel - 1 + count) % count; appSetMenuSelected(sel);
                }
            }
            if (pressed & 0x02) { // DOWN
                blinkNeo(40, 0, 0);
                if (appGetCurrentView() == UIView::VIEW_NOW_PLAYING) {
                    audioSetVolume(max(0, audioGetVolume() - 5));
                    char buf[32]; snprintf(buf, sizeof(buf), "Vol: %d%%", audioGetVolume()); uiToast(buf);
                } else {
                    int count = (appGetMenuLevel() == 0 ? 3 : 4);
                    int sel = appGetMenuSelected(); sel = (sel + 1) % count; appSetMenuSelected(sel);
                }
            }
            if (pressed & 0x04) { // SELECT pressed: start hold tracking only
                selPressMs = nowMs; selHeld = true; selHoldTriggered = false;
            }
            if (released & 0x04) {
                // If not a long-press, treat as short-press select
                if (!selHoldTriggered) {
                    blinkNeo(40, 40, 0);
                    if (appGetCurrentView() == UIView::VIEW_MENU) {
                        if (appGetMenuLevel() == 0) {
                            if (appGetMenuSelected() == 0) { appSetMenuLevel(1); appSetMenuSelected(0); }
                            else if (appGetMenuSelected() == 1) uiToast("RFID: placeholder");
                            else if (appGetMenuSelected() == 2) uiToast("Settings: placeholder");
                        } else {
                            if (appGetMenuSelected() == 0) { appSetCurrentView(UIView::VIEW_NOW_PLAYING); appResetNowPlayingSeconds(); }
                            else uiToast("Selected item");
                        }
                    }
                }
                selHeld = false; selHoldTriggered = false;
            }
            if (pressed & 0x08) { // BACK (menu)
                blinkNeo(40, 0, 40);
                if (appGetCurrentView() == UIView::VIEW_NOW_PLAYING) appSetCurrentView(UIView::VIEW_MENU);
                else if (appGetCurrentView() == UIView::VIEW_MENU && appGetMenuLevel() == 1) { appSetMenuLevel(0); appSetMenuSelected(0); }
            }
            if (pressed & 0x10) { // PLAY/PAUSE
                blinkNeo(20, 20, 20);
                audioSetPlaying(!audioIsPlaying());
                uiToast(audioIsPlaying() ? "Audio: Play" : "Audio: Pause");
            }
            last = cur;
            lastDebounceMs = nowMs;
        }

        // Long-press SELECT -> Now Playing (evaluated continuously while held)
        if (selHeld && !selHoldTriggered) {
            if (millis() - selPressMs > 600) {
                blinkNeo(0, 40, 40);
                appSetCurrentView(UIView::VIEW_NOW_PLAYING);
                appResetNowPlayingSeconds();
                selHoldTriggered = true;
            }
        }

        // Apply touch wheel scroll to menu/volume
        int wheelSteps = touchWheelGetAndClearDelta();
        if (wheelSteps != 0) {
            g_wheelDebugCounter += wheelSteps; // Update debug counter
            Serial.printf("Wheel steps: %d, debug counter now: %d\n", wheelSteps, g_wheelDebugCounter);
            // Force wheel counter redraw immediately
            drawWheelCounter();
            int uiMoves = wheelSteps; // 1:1 mapping for responsive feel
            if (appGetCurrentView() == UIView::VIEW_NOW_PLAYING) {
                int vol = audioGetVolume();
                vol = constrain(vol + uiMoves * 2, 0, 100);
                audioSetVolume(vol);
                char buf[32]; snprintf(buf, sizeof(buf), "Vol: %d%%", vol); uiToast(buf);
            } else {
                int count = (appGetMenuLevel() == 0 ? 3 : 4);
                int sel = appGetMenuSelected();
                sel = (sel + (uiMoves % count) + count) % count;
                appSetMenuSelected(sel);
                char buf[24]; snprintf(buf, sizeof(buf), "Menu sel: %d", sel);
                uiToast(buf);
            }
        }

        if (Serial.available()) {
            char c = (char)Serial.read();
            if (c == 'u' || c == 'U') {
                int count = (appGetMenuLevel() == 0 ? 3 : 4);
                int sel = appGetMenuSelected();
                sel = (sel - 1 + count) % count;
                appSetMenuSelected(sel);
            } else if (c == 'd' || c == 'D') {
                int count = (appGetMenuLevel() == 0 ? 3 : 4);
                int sel = appGetMenuSelected();
                sel = (sel + 1) % count;
                appSetMenuSelected(sel);
            } else if (c == 's' || c == 'S' || c == '\n') {
                if (appGetCurrentView() == UIView::VIEW_MENU) {
                    if (appGetMenuLevel() == 0) {
                        if (appGetMenuSelected() == 0) { // Music
                            appSetMenuLevel(1);
                            appSetMenuSelected(0);
                        } else if (appGetMenuSelected() == 1) {
                            uiToast("RFID: placeholder");
                        } else if (appGetMenuSelected() == 2) {
                            uiToast("Settings: placeholder");
                        }
                    } else {
                        if (appGetMenuSelected() == 0) { // Now Playing
                            appSetCurrentView(UIView::VIEW_NOW_PLAYING);
                            appResetNowPlayingSeconds();
                        } else {
                            uiToast("Selected item");
                        }
                    }
                }
            } else if (c == 'b' || c == 'B') {
                if (appGetCurrentView() == UIView::VIEW_NOW_PLAYING) {
                    appSetCurrentView(UIView::VIEW_MENU);
                } else if (appGetCurrentView() == UIView::VIEW_MENU && appGetMenuLevel() == 1) {
                    appSetMenuLevel(0);
                    appSetMenuSelected(0);
                }
            } else if (c == 'p' || c == 'P') {
                audioSetPlaying(!audioIsPlaying());
                uiToast(audioIsPlaying() ? "Audio: Play" : "Audio: Pause");
            } else if (c == ' ') {
                audioSetPlaying(!audioIsPlaying());
                uiToast(audioIsPlaying() ? "Audio: Play" : "Audio: Pause");
            } else if (c == 'n' || c == 'N') {
                appNextTrack();
                if (appGetCurrentView() == UIView::VIEW_NOW_PLAYING) appRequestRedraw();
                uiToast("Next track");
            } else if (c == 'r' || c == 'R') {
                appPrevTrack();
                if (appGetCurrentView() == UIView::VIEW_NOW_PLAYING) appRequestRedraw();
                uiToast("Prev track");
            } else if (c == '+') {
                audioSetVolume(min(100, audioGetVolume() + 5));
                char buf[32]; snprintf(buf, sizeof(buf), "Volume: %d%%", audioGetVolume());
                uiToast(buf);
            } else if (c == '-') {
                audioSetVolume(max(0, audioGetVolume() - 5));
                char buf[32]; snprintf(buf, sizeof(buf), "Volume: %d%%", audioGetVolume());
                uiToast(buf);
            } else if (c == 'w') {
                if (g_sdMounted) {
                    if (wavStartFirstUnderMusic()) { uiToast("Playing WAV from /Music"); }
                    else { uiToast("No 44.1kHz 16b stereo WAV found"); }
                }
            } else if (c == 'x') {
                wavStop(); uiToast("Stop WAV");
            } else if (c == 'm') {
                if (g_sdMounted) {
                    wavStop(); audioSetPlaying(false);
                    Serial.println("MP3: attempting to start first file under /Music...");
                    if (mp3StartFirstUnderMusic()) { mp3EnsureTask(); uiToast("Playing MP3 from /Music"); }
                    else { uiToast("No MP3 found"); }
                }
            } else if (c == 'q') {
                mp3Stop(); uiToast("Stop MP3");
            } else if (c == 'S') {
                // Re-list SD
                if (g_sdMounted) { listSdFiles("/"); if (SD.exists("/Music")) listSdFiles("/Music"); }
            } else if (c == 'F') {
                // Force SD layout creation
                if (g_sdMounted) { initSdLayout(); Serial.println("SD: layout ensured"); }
            } else if (c == 'X') {
                // Quick-format SD (delete all, recreate structure)
                if (g_sdMounted) {
                    if (sdQuickFormat()) { listSdFiles("/"); }
                }
            } else if (c == 'T') {
                // Toggle raw touch debug
                bool en = !touchWheelGetDebugRaw();
                touchWheelSetDebugRaw(en);
                Serial.printf("TouchWheel raw debug: %s\n", en ? "ON" : "OFF");
            } else if (c == 'I') {
                // Invert wheel direction
                static bool inv = false; inv = !inv; touchWheelSetInvert(inv);
                Serial.printf("TouchWheel invert: %s\n", inv ? "ON" : "OFF");
            } else if (c == 'W') {
                // Manual wheel test: increment counter
                g_wheelDebugCounter++;
                Serial.printf("Manual wheel test: counter now %d\n", g_wheelDebugCounter);
                drawWheelCounter(); // Force redraw
            } else if (c == 'E') {
                // Show electrode order
                Serial.print("Electrode order: ");
                for (int i = 0; i < 12; i++) Serial.printf("%d ", i);
                Serial.println();
                Serial.printf("Connected: %s\n", touchWheelIsConnected() ? "YES" : "NO");
                uint8_t touch, release;
                touchWheelGetThresholds(&touch, &release);
                Serial.printf("Thresholds: touch=%d release=%d\n", touch, release);
            } else if (c == 'Q') {
                // Quick test: inject a fake wheel step to test the UI pipeline
                g_wheelDebugCounter += 1;
                Serial.printf("Manual step injection: counter now %d\n", g_wheelDebugCounter);
                drawWheelCounter();
                // Also test the menu logic
                int count = (appGetMenuLevel() == 0 ? 3 : 4);
                int sel = appGetMenuSelected();
                sel = (sel + 1) % count;
                appSetMenuSelected(sel);
                char buf[24]; snprintf(buf, sizeof(buf), "Menu sel: %d", sel);
                uiToast(buf);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void setup() {
    Serial.begin(115200);
    delay(10000); // delay 10s to allow opening Serial Monitor before logs start
    pinMode(LED_BUILTIN, OUTPUT);

    displayMutex = xSemaphoreCreateMutex();

    // Audio
    if (audioInit()) {
        Serial.println("Audio ready: 'p' play/pause, '+'/'-' volume");
        g_audioReady = true;
    } else {
        Serial.println("Audio I2S init failed");
        g_audioReady = false;
    }

    // NeoPixel init
    pinMode(NEOPIXEL_PWR, OUTPUT);
    digitalWrite(NEOPIXEL_PWR, HIGH); // enable power
    neopix.begin(); neopix.clear(); neopix.show();
    blinkNeo(0, 50, 0, 60);

    // SD (SPI shared): attach MISO and bring up
    SPI.begin(TFT_SCLK, SD_MISO, TFT_MOSI, SD_CS);
    if (SD.begin(SD_CS, SPI, 5000000)) {
        Serial.println("SD: mounted OK (SPI)");
        g_sdMounted = true;
        if (!sdLayoutValid()) {
            Serial.println("SD: layout not detected. Press 'X' to quick-format and create folders, or 'F' to only create missing folders.");
        } else {
            initSdLayout(); // ensure any missing subfolders
        }
        Serial.println("SD: listing / and /Music if present");
        listSdFiles("/");
        if (SD.exists("/Music")) listSdFiles("/Music");
    } else {
        Serial.println("SD: mount FAILED");
        g_sdMounted = false;
    }

    // Aux UART on RX=38 TX=39 (if you connect a device)
    initAuxUart();

    // Tasks
    xTaskCreatePinnedToCore(displayTask, "DisplayTask", 8192, NULL, 2, &displayTaskHandle, 1);
    // xTaskCreatePinnedToCore(heartbeatTask, "HeartbeatTask", 4096, NULL, 1, &heartbeatTaskHandle, 0); // disabled per request
    xTaskCreatePinnedToCore(animationTask, "AnimationTask", 4096, NULL, 1, &animationTaskHandle, 0);
    xTaskCreatePinnedToCore(menuTask, "MenuTask", 4096, NULL, 1, &menuTaskHandle, 1);
    xTaskCreatePinnedToCore(statusTask, "StatusTask", 3072, NULL, 1, &statusTaskHandle, 1);

    // Initial state
    appSetCurrentView(UIView::VIEW_MENU);
    appSetMenuLevel(0);
    appSetMenuSelected(0);

    Serial.println("Controls: u/d/s select, b back, p play/pause, +/- volume");
    Serial.printf("Firmware: %s %s\n", izod_firmware_name(), izod_firmware_version());
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
