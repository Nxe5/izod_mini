# iZod Mini — ESP32-S3 iPod‑Inspired Music + Pentest Device

iZod Mini is a portable, open project inspired by the iPod Nano (3rd gen) experience, built on ESP32‑S3. The goal is to deliver a smooth iPod‑like UI (click/touch wheel, album art, fast library browsing) with high‑quality audio playback, then expand into pentest tooling. A larger “Classic” variant will further add sub‑GHz (CC1101), IR blaster, and nRF24 capabilities.

- Nano focus (initial): ST7789 display, touch wheel, SD music, Wi‑Fi/BLE, LF RFID (read/write/emulate)
- Classic expansion (later): CC1101, IR, nRF24 modules in addition to Nano features

## Features (Planned by Phases)
- Music foundation: ST7789 UI, click/touch wheel, SD (4‑bit) library, PCM5102 I2S audio
- LF RFID: read, save, emulate workflows for simple access use‑cases
- Web + OTA: local web UI for settings and OTA; GitHub Releases driven updates
- Third‑party apps: simple app library + installer, sandboxed APIs
- Wireless expansion (Classic): CC1101 (sub‑GHz), IR blaster, nRF24 tools
- Signed firmware and apps: provenance indicators for “official” vs third‑party

See detailed phase docs under `docs/phases/`.

## Hardware
- MCU: ESP32‑S3 Feather‑class board
- Display: ST7789 (240×280) over SPI
- Audio: PCM5102 I2S DAC (44.1kHz+), headphone out
- Storage: SD card via SDMMC 4‑bit mode (fast)
- Inputs: TouchWheel0 custom wheel (and/or MPR121), basic buttons
- Radios (later on Classic): CC1101, nRF24L01, IR blaster/receiver

Initial display pinout (breadboard prototype):
- CS=12, DC=13, SCK=36, MOSI=35, RST=−1, BL(optional)
- I2S: BCLK=1, WS/LRCK=2, DATA=42
- SDMMC 4‑bit (baseline wiring, see docs for updates): CMD=38, CLK=47, D0=14 (expand to D1/D2/D3 on PCB)

Refer to `docs/phases/01-music-foundation.md` for current wiring and bring‑up notes.

## Software Stack
- Framework: Arduino + FreeRTOS (multitasking for UI/audio); selective ESP‑IDF components later as needed
- Display: Adafruit_GFX + Adafruit_ST7789
- Storage: SD_MMC (4‑bit)
- Audio: I2S → PCM5102

## Release Signing & Provenance
- Firmware bundles include a `firmware.json` manifest (name, version, build date, git commit, env, binary filename).
- A developer public key is embedded on‑device; release artifacts are signed (e.g., Ed25519) so devices can verify official builds.
- UI shows a trust badge: “Official” (valid signature) vs “Third‑party” (no/invalid signature). Third‑party is allowed but visually distinct.
- App packages follow the same pattern: `app.json` + signature; installer verifies and labels provenance.
- Key policy: reuse an org/year key name like `ocho-labs-release-2025`; rotate yearly. See `docs/references/signing.md`.

## Quick Start (PlatformIO)
1. Open in VS Code with PlatformIO
2. Environment: `esp32s3_feather_breadboard`
3. Wire display per docs, connect board
4. Build & upload: `pio run --target upload`
5. Monitor: `pio device monitor`

## UI Concept
- Split view layout: left = text lists, right = artwork/video (placeholder initially)
- Smooth scrolling with minimal redraws (dirty rectangles)
- Now Playing: metadata, progress, transport controls

## Roadmap
- Phase 01: Music Foundation (display, wheel/buttons, SD, audio)
- Phase 02: LF RFID (read/write/emulate, tag library)
- Phase 03: Web UI + OTA + GitHub Releases + signed firmware
- Phase 04: Third‑party Apps Platform + signed apps + trust badges
- Phase 05: Wireless Expansion (CC1101, IR, nRF24)
- Phase 06: UI/UX Design polish; evaluate LVGL later
- Phase 07: Storage & Indexing (SDMMC 4‑bit, metadata cache)
- Phase 08: Hardware Integration & Power
- Phase 09: Security & Pentest (Wi‑Fi/BLE)
- Phase 10: Next Displays & Possible ESP‑IDF migrations; key rotation and trust store management

Details in `docs/phases/`.

## Inspirations & References
- Tangara (ESP32 portable music player) — professional open design: [tangara‑fw](https://github.com/Gigahawk/tangara-fw)
- ESP32 pentest toolkit: [ESP32‑DIV](https://github.com/cifertech/ESP32-DIV)
- nRF24 tools and examples: [nRFBox](https://github.com/cifertech/nRFBox)
- Multi‑function pentest device ideas: [EvilDuck](https://github.com/cifertech/EvilDuck)
- 2.4 GHz scanning background (Arduino Forum): [Poor man’s 2.4 GHz scanner](https://forum.arduino.cc/t/poor-mans-2-4-ghz-scanner/54846)
- Menu patterns/UI inspiration: [arduino_oled_menu](https://github.com/upiir/arduino_oled_menu)
- nRF24 Wi‑Fi jammer (research/education only): [nRF24L01‑WiFi‑Jammer](https://github.com/hugorezende/nRF24L01-WiFi-Jammer)
- RC system ideas and codebase: [Universal‑RC‑system](https://github.com/alexbeliaev/Universal-RC-system/tree/master)
- BLE security / Apple research: [ESP32‑Sour‑Apple](https://github.com/RapierXbox/ESP32-Sour-Apple)
- Reference pentest UX and app model: [flipperzero‑firmware](https://github.com/flipperdevices/flipperzero-firmware)

## Contributing
- See `docs/README.md` and the `phases/` documents
- PRs welcome for docs, wiring, driver bring‑up, UI components, and test harnesses

## License & Legal
- License: TBD
- Pentest features are for lawful, educational, or lab‑only use. Ensure compliance with local laws.
