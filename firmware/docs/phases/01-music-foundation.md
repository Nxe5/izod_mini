# Phase 01 — Music Foundation (Display, Wheel, Playback)

Goal: Deliver an iPod‑like experience: boot, draw UI, browse SD music, play audio, responsive wheel/buttons.

## Scope
- ST7789 display (240×280) with Adafruit_GFX/Adafruit_ST7789
- Touch/click wheel (TouchWheel0 custom; later optional MPR121)
- SD card in 4‑bit SDMMC mode
- I2S DAC (PCM5102) audio playback
- FreeRTOS task model for smooth UI + audio
- iPod‑style split view (left text, right artwork)

## Architecture
- Arduino + FreeRTOS
- Tasks
  - Display/UI Task (Core 1): 20–30 FPS UI updates, draw lists/artwork
  - Input Task (Core 1): wheel/buttons debounced events → queue
  - Audio Task (Core 0): I2S stream, buffer mgmt, decoder
  - Filesystem Task (Core 0): directory scan, metadata (ID3/FLAC tags)
- Queues/Semaphores
  - input_queue: events (up/down/left/right/select/back)
  - ui_queue: UI commands (navigate, show artwork, toast)
  - audio_queue: play/pause/next/prev/volume

## Wiring (Initial)
- ST7789 (SPI): CS=12, DC=13, SCK=36, MOSI=35, RST=-1, BL=optional
- PCM5102 (I2S): BCLK=1, LRCK/WS=2, DATA=42
- SD (SDMMC 4‑bit): CMD=38, CLK=47, D0=14 (expand with D1/D2/D3 on future PCB)
- TouchWheel0/MPR121: I2C SDA=3, SCL=4 (prototyping)

## Features
- UI
  - Home → Music → Artists/Albums/Songs → Now Playing
  - Split view: left list, right artwork (placeholder if none)
  - Status bar: battery (mock), Wi‑Fi, time (mock), bitrate
- Input
  - Wheel/Buttons: scroll, select, back, hold
  - Long‑press mappings (e.g., add to queue)
- Audio
  - File formats: start with WAV/MP3; later FLAC/OGG/AAC
  - Gapless not required initially
  - Volume, track seek, next/prev
- Storage
  - SD structure: /Music/Artist/Album/track.ext
  - Artwork lookup: folder.jpg, embedded tags later

## Acceptance Criteria
- Boots to Home in < 2s after splash
- Scroll lists at 30 FPS; no stutter while audio plays
- Plays 44.1kHz stereo MP3/WAV via PCM5102
- No audio underruns when navigating
- SD scan under 5s for 500 songs (indexed cache acceptable)

## Libraries & References
- Display/UI: Adafruit_GFX, Adafruit_ST7789; later LVGL option; inspiration: [tangara-fw](https://github.com/Gigahawk/tangara-fw)
- Audio: ESP32 I2S (Arduino or ESP-IDF via Arduino); candidates: ESP32-AudioI2S, Arduino Audio Tools
- Storage: SD_MMC 4‑bit (Arduino SD_MMC, or ESP-IDF layer)
- Menu patterns: [arduino_oled_menu](https://github.com/upiir/arduino_oled_menu)

## Implementation Steps
1. Display bring‑up (Hello World, rotations, basic widgets)
2. Input driver: TouchWheel0 prototype (position, tap, long‑press); fallback button matrix
3. SD_MMC in 4‑bit mode; mount and list directory
4. Metadata extraction (basic ID3)
5. Audio pipeline: file → decoder → I2S → DAC; circular buffers
6. Now Playing screen + transport controls
7. Library views (Artists/Albums/All Songs)
8. Basic settings (volume, brightness)

## Notes
- Prefer FreeRTOS queues over globals; keep ISRs minimal
- Draw diff regions to minimize SPI bandwidth (no full screen clears)
- Reserve PSRAM if used; budget display + audio buffers carefully
