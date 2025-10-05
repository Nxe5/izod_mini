#include "ui_display.h"
#include <functional>

static Adafruit_ST7789* s_display = nullptr;
static SemaphoreHandle_t* s_mutex = nullptr;

static const int kWidth = 240;
static const int kHeight = 280;

void uiInit(Adafruit_ST7789* d, SemaphoreHandle_t* mtx) {
    s_display = d;
    s_mutex = mtx;
}

static void withLock(std::function<void()> fn) {
    if (!s_display || !s_mutex) return;
    if (xSemaphoreTake(*s_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return;
    fn();
    xSemaphoreGive(*s_mutex);
}

void uiDrawHome() {
    withLock([](){
        s_display->fillScreen(UI_COLOR_BG);
        s_display->setTextColor(UI_COLOR_FG);
        s_display->setTextSize(2);
        s_display->setCursor(10, 10);
        s_display->println("Home");
        s_display->drawFastHLine(10, 35, 220, UI_COLOR_HI);

        static const char* items[] = {"Music", "RFID", "Settings"};
        int count = 3;
        int sel = appGetMenuSelected();
        int y = 50;
        for (int i = 0; i < count; i++) {
            if (i == sel) {
                s_display->fillRect(8, y - 2, 224, 20, UI_COLOR_ACCENT);
                s_display->setTextColor(UI_COLOR_BG);
            } else {
                s_display->setTextColor(UI_COLOR_FG);
            }
            s_display->setTextSize(1);
            s_display->setCursor(12, y);
            s_display->println(items[i]);
            y += 22;
        }
        s_display->drawRect(150, 50, 80, 80, UI_COLOR_FG);
        s_display->setCursor(160, 88);
        s_display->setTextColor(UI_COLOR_FG);
        s_display->setTextSize(1);
        s_display->println("menu");
    });
}

void uiDrawMusic() {
    withLock([](){
        s_display->fillScreen(UI_COLOR_BG);
        s_display->setTextColor(UI_COLOR_FG);
        s_display->setTextSize(2);
        s_display->setCursor(10, 10);
        s_display->println("Music");
        s_display->drawFastHLine(10, 35, 220, UI_COLOR_HI);

        static const char* items[] = {"Now Playing", "Artists", "Albums", "Songs"};
        int count = 4;
        int sel = appGetMenuSelected();
        int y = 50;
        for (int i = 0; i < count; i++) {
            if (i == sel) {
                s_display->fillRect(8, y - 2, 224, 20, UI_COLOR_ACCENT);
                s_display->setTextColor(UI_COLOR_BG);
            } else {
                s_display->setTextColor(UI_COLOR_FG);
            }
            s_display->setTextSize(1);
            s_display->setCursor(12, y);
            s_display->println(items[i]);
            y += 22;
        }
        s_display->drawRect(150, 50, 80, 80, UI_COLOR_FG);
        s_display->setCursor(155, 88);
        s_display->setTextColor(UI_COLOR_FG);
        s_display->setTextSize(1);
        s_display->println("artwork");
    });
}

void uiDrawNowPlayingFull() {
    withLock([](){
        s_display->fillScreen(UI_COLOR_BG);
        s_display->setTextColor(UI_COLOR_FG);
        s_display->setTextSize(2);
        s_display->setCursor(10, 10);
        s_display->println("Now Playing");
        s_display->drawFastHLine(10, 35, 220, UI_COLOR_HI);

        s_display->drawRect(10, 50, 100, 100, UI_COLOR_FG);
        s_display->setCursor(25, 98);
        s_display->setTextSize(1);
        s_display->println("artwork");

        s_display->setTextSize(1);
        s_display->setCursor(120, 60);
        s_display->println(appGetCurrentTrackTitle());
        s_display->setCursor(120, 75);
        s_display->println(appGetCurrentTrackArtist());

        s_display->drawRect(10, 170, 220, 12, UI_COLOR_FG);

        // Duration at right
        int dur = appGetCurrentTrackDurationSec();
        int mm = dur / 60, ss = dur % 60;
        s_display->setCursor(200, 190);
        char buf[8]; snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss);
        s_display->print(buf);
    });
    uiUpdateNowPlayingProgress();
}

void uiUpdateNowPlayingProgress() {
    withLock([](){
        int secs = appGetNowPlayingSeconds();
        int prog = (secs % appGetCurrentTrackDurationSec()) * (220 - 4) / max(1, appGetCurrentTrackDurationSec());
        s_display->fillRect(12, 172, 216, 8, UI_COLOR_BG);
        s_display->fillRect(12, 172, prog, 8, UI_COLOR_HI);
        s_display->fillRect(10, 188, 60, 12, UI_COLOR_BG);
        s_display->setTextColor(UI_COLOR_FG);
        s_display->setTextSize(1);
        s_display->setCursor(10, 190);
        s_display->printf("%02d:%02d", secs / 60, secs % 60);
    });
}

void uiToast(const char* msg) {
    withLock([&](){
        s_display->fillRect(10, 38, 220, 12, UI_COLOR_BG);
        s_display->setTextColor(UI_COLOR_HI);
        s_display->setTextSize(1);
        s_display->setCursor(10, 38);
        s_display->print(msg);
    });
}

void uiShowSplash(const char* company, const char* fwName, const char* fwVersion, const char* badgeText, uint16_t badgeColor) {
    withLock([&](){
        s_display->fillScreen(UI_COLOR_BG);
        // Company (logo placeholder)
        s_display->setTextColor(UI_COLOR_FG);
        s_display->setTextSize(2);
        int cx = 20, cy = 40;
        s_display->setCursor(cx, cy);
        s_display->print(company);

        // Firmware name
        s_display->setTextSize(2);
        s_display->setCursor(20, cy + 40);
        s_display->print(fwName);

        // Version
        s_display->setTextSize(1);
        s_display->setCursor(20, cy + 65);
        s_display->print("Version: ");
        s_display->print(fwVersion);

        // Badge (optional)
        if (badgeText && badgeText[0]) {
            s_display->setTextColor(badgeColor);
            s_display->setTextSize(1);
            s_display->setCursor(20, cy + 85);
            s_display->print(badgeText);
        }
    });
}
