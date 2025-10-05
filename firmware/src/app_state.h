#pragma once
#include <stdint.h>

enum class UIView : uint8_t { VIEW_MENU = 0, VIEW_NOW_PLAYING = 1 };

// State accessors
UIView appGetCurrentView();
void appSetCurrentView(UIView v);

int appGetMenuLevel();               // 0 = Home, 1 = Music
void appSetMenuLevel(int level);

int appGetMenuSelected();
void appSetMenuSelected(int sel);

int appGetNowPlayingSeconds();
void appResetNowPlayingSeconds();
void appIncrementNowPlayingSecondsMod(int modSeconds);

// Redraw control
void appRequestRedraw();
bool appConsumeRedrawRequest();

// Mock track library
int appGetTrackCount();
int appGetCurrentTrackIndex();
void appNextTrack();
void appPrevTrack();
const char* appGetCurrentTrackTitle();
const char* appGetCurrentTrackArtist();
int appGetCurrentTrackDurationSec();
