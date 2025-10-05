#include "app_state.h"
#include <Arduino.h>

static volatile UIView s_currentView = UIView::VIEW_MENU;
static volatile int s_menuLevel = 0;
static volatile int s_menuSelected = 0;
static volatile int s_nowPlayingSeconds = 0;
static volatile bool s_forceRedraw = true;

// Mock library
struct Track { const char* title; const char* artist; int durationSec; };
static const Track kTracks[] = {
    {"Test Tone", "iZod", 180},
    {"Demo Song A", "Artist 1", 210},
    {"Demo Song B", "Artist 2", 195},
    {"Demo Song C", "Artist 3", 240},
    {"Demo Song D", "Artist 4", 175}
};
static volatile int s_trackIndex = 0;

UIView appGetCurrentView() { return s_currentView; }
void appSetCurrentView(UIView v) { s_currentView = v; s_forceRedraw = true; }

int appGetMenuLevel() { return s_menuLevel; }
void appSetMenuLevel(int level) { s_menuLevel = level; s_forceRedraw = true; }

int appGetMenuSelected() { return s_menuSelected; }
void appSetMenuSelected(int sel) { s_menuSelected = sel; s_forceRedraw = true; }

int appGetNowPlayingSeconds() { return s_nowPlayingSeconds; }
void appResetNowPlayingSeconds() { s_nowPlayingSeconds = 0; s_forceRedraw = true; }
void appIncrementNowPlayingSecondsMod(int modSeconds) {
    if (modSeconds <= 0) return;
    int v = s_nowPlayingSeconds + 1;
    if (v >= modSeconds) v = 0;
    s_nowPlayingSeconds = v;
}

void appRequestRedraw() { s_forceRedraw = true; }
bool appConsumeRedrawRequest() { bool r = s_forceRedraw; s_forceRedraw = false; return r; }

int appGetTrackCount() { return (int)(sizeof(kTracks)/sizeof(kTracks[0])); }
int appGetCurrentTrackIndex() { return s_trackIndex; }
void appNextTrack() { s_trackIndex = (s_trackIndex + 1) % appGetTrackCount(); s_forceRedraw = true; appResetNowPlayingSeconds(); }
void appPrevTrack() { s_trackIndex = (s_trackIndex - 1 + appGetTrackCount()) % appGetTrackCount(); s_forceRedraw = true; appResetNowPlayingSeconds(); }
const char* appGetCurrentTrackTitle() { return kTracks[s_trackIndex].title; }
const char* appGetCurrentTrackArtist() { return kTracks[s_trackIndex].artist; }
int appGetCurrentTrackDurationSec() { return kTracks[s_trackIndex].durationSec; }
