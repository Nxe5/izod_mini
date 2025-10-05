#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "freertos/semphr.h"
#include "app_state.h"

// Colors
#define UI_COLOR_BG      0x0000
#define UI_COLOR_FG      0xFFFF
#define UI_COLOR_HI      0x07E0
#define UI_COLOR_ACCENT  0xFBE0

void uiInit(Adafruit_ST7789* d, SemaphoreHandle_t* mtx);

void uiDrawHome();
void uiDrawMusic();
void uiDrawNowPlayingFull();
void uiUpdateNowPlayingProgress();

void uiToast(const char* msg);

// Splash screen
void uiShowSplash(const char* company, const char* fwName, const char* fwVersion, const char* badgeText, uint16_t badgeColor);
