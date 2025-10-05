# iZod Mini Plan (Nano‑style)

Comprehensive checklist to ship the Nano variant: music‑first device with ST7789, touch/click wheel, SD (optional initially), Wi‑Fi/BLE basics, and LF RFID.

### Overall Progress
- 0) Foundations — ~95%
- 1) Display & UI — ~55%
- 2) Input (Wheel/Buttons) — ~95% (wheel proto working)
- 3) Audio — ~60% (FLAC 0%)
- 4) Storage — ~30%
- 5) LF RFID — ~5%
- 6) Web + OTA — ~15%
- 7) Apps Platform — ~5%
- 8) QA & Polish — ~5%
- 9) Release & Signing — ~35%
- 10) Connectivity & Pentest — ~0%

Overall completion: ~48%

## 0) Foundations
- [x] Confirm board: ESP32‑S3 Feather — 100%
- [x] PlatformIO env set; Arduino + FreeRTOS — 100%
- [x] Pin map documented in README + code — 100%
- [ ] Serial logging + error macros — 80%
 - [ ] Chip decision for Mini: migrate to ESP32‑PICO‑V3‑02 (Classic BT + compact SiP) — 0%
 - Note: Development remains on ESP32‑S3‑Mini; PICO migration is a later phase

## Controls
- Target iPod‑style controls:
  - Buttons (5): Menu (back), Play/Pause, Previous, Next, Select (center) — 100%
  - Wheel (MPR121): segmented ring → scroll menus; volume in Now Playing — 60% (prototype working; needs tuning/PCB)
  - Long‑press Select: jump to Now Playing from anywhere — 90%
  - Default volume: 50% — 100%
- Temporary (dev):
  - Serial keys (u/d/s/b, space/p, n/r, +/-) — 100%
  - 5 GPIO buttons allowed; current prototype uses GPIO 14/15/16/17/18 (mapping subject to change to iPod layout) — 100%

## 1) Display & UI
- [x] ST7789 wired and initialized (pins: CS=12, DC=13, SCK=36, MOSI=35, RST=−1) — 100%
- [ ] Basic widgets: list, status bar, artwork pane — 60%
- [ ] Split view layout (left text, right image) — 30%
- [ ] Frame pacing (20–30 FPS) and dirty rect redraws — 60%
- [ ] Theme & fonts selected — 20%

## 2) Input (Wheel/Buttons)
- [x] 5-button mapping implemented (Menu/Play/Prev/Next/Select) — 100%
- [ ] Select long‑press → Now Playing — 90%
- [ ] Wheel (MPR121 12‑pad): COM interpolation, step accumulation, inversion, electrode order — 60%
- [ ] Debounce and event model; minimal logging — 80%
  - Notes: Breadboard proto sensitive to wiring; anti‑drift gating + on‑screen counter added; use PCB wheel with ground ring/overlay

## 3) Audio
- [x] PCM5102 I2S bring‑up (BCLK=1, LRCK=2, DATA=42) — 100%
- [x] Test tones and WAV playback — 100%
- [x] MP3 decode (library) playback from SD — 100%  
  (ring buffer optimization and long‑run soak tests later)
- [ ] FLAC decode (library) with streaming from SD; verify 44.1/48 kHz stereo — 0%
- [x] Volume control API — 100%
- [ ] Underrun detection — 0%

## 4) Storage (Optional First Pass)
- [ ] SDMMC 4‑bit wiring verified — 10%
- [x] Mount SD and iterate directory — 100%
- [ ] Metadata (ID3) minimal parse — 0%
- [ ] Library index cache on SD — 0%
 - [ ] Decide storage for Mini: low‑profile microSD (removable) vs eMMC (soldered) vs SPI NAND (fixed) — 0%
   - microSD: easiest, user‑expandable; socket height ~1.6–1.9 mm
   - eMMC (4‑bit via SDMMC): thinnest (BGA, ~1.0 mm), high capacity; firmware integration via ESP‑IDF SDMMC
   - SPI NAND (1–2 Gb): smallest WSON8, fixed library; requires wear‑leveling/FTL and FATFS integration
 - [ ] Prototype: keep microSD (SPI now; migrate to SDMMC 4‑bit) — 0%
 - [ ] Production Option A: eMMC footprint + bring‑up (SDMMC 4‑bit) — 0%
 - [ ] Production Option B: SPI NAND footprint + FTL/wear‑leveling plan — 0%

Tip: Phases 1–3 can run without SD by using embedded assets or test buffers.

## 5) LF RFID
- [ ] Select LF front‑end/module and pinout — 40%
- [ ] RP2040 + EM4095 architecture for 125 kHz (R/W/Emulate) — 30%
  - EM4095: coil + LC tune to 125 kHz; DEMOD→RP2040; MOD←RP2040
  - RP2040 (PIO): 125 kHz carrier + Manchester/ASK encode/decode; TinyUSB CDC+HID
  - UART framing ESP32↔RP2040 (LF_READ, LF_WRITE_T5577, LF_EMULATE_START/STOP)
- [ ] Read EM4100/HID Prox; display ID; save to SD — 0%
- [ ] Write T5577; verify with reader — 0%
- [ ] Emulate EM4100/HID from RAM/SD — 0%
- [ ] UI: read → details → save; write/emulate picker — 0%

## 6) Web + OTA
- [ ] Async web server skeleton + auth — 0%
- [ ] Settings pages (JSON API) — 0%
- [ ] OTA from upload and GitHub Release — 0%
- [ ] Version displayed in UI — 70%

## 7) Apps Platform (Light)
- [ ] App manifest (JSON) — 10%
- [ ] Simple demo app lifecycle — 0%
- [ ] Installer from SD/URL — 0%

## 8) QA & Polish
- [ ] Boot time < 2s to Home — 10%
- [ ] Smooth scroll while audio playing — 0%
- [ ] Power use measured; thermal check — 0%
- [ ] Crash logs; watchdog handling — 0%

## 9) Release & Signing
- [x] `firmware.json` manifest generated on build — 100%
- [ ] `firmware.sig` created in CI (developer private key) — 20%
- [ ] Device embeds developer public key — 30%
- [ ] On-device signature verification (Ed25519) — 0%
- [ ] UI badge: Official vs Third‑party; override policy documented — 30%

## 10) Connectivity & Pentest (Wi‑Fi/BLE/USB HID)
- Wi‑Fi (legal/ethical, opt‑in only)
  - [ ] Scan and list nearby APs (SSID, RSSI, channel) — 0%
  - [ ] Promiscuous/monitor mode capture to PCAP on SD (where permitted) — 0%
  - [ ] Status UI: Wi‑Fi health and MAC info — 0%
- BLE (legal/ethical, opt‑in only)
  - [ ] BLE scan (name, RSSI, services) — 0%
  - [ ] GATT client demo (read/write a characteristic) — 0%
  - [ ] Advertising demo with user‑set name — 0%
- USB HID “BadUSB” (safety gates)
  - [ ] Deferred to later phase (USB coprocessor) — 0%
  - [ ] Coprocessor choice: RP2040 + TinyUSB (CDC + HID; optional MSC) — 0%
  - [ ] Define UART command protocol ESP32↔RP2040 (KEYSEQ/MOUSE/MSC) — 100% (see Appendix)
  - [ ] Safety: LED indicator, cancel key, rate limits — 0%
- Safety & Compliance
  - [ ] Legal notice, explicit enable toggles, logging — 0%

## References
- Tangara: https://github.com/Gigahawk/tangara-fw
- ESP32-DIV: https://github.com/cifertech/ESP32-DIV
- UI menu ideas: https://github.com/upiir/arduino_oled_menu

## Current Wiring (Prototype)
- Display ST7789 (SPI): CS12, DC13, SCK36, MOSI35, RST−1, BL6
- Audio PCM5102A (I2S): BCLK39, WS/LRCK38, DATA11, VCC, GND (MCLK not used)
- SD (SPI mode): SCK36, MOSI35, MISO37, CS8, 3V3, GND
- Buttons (5): Up14, Down15, Select16, Back17, Play18 (INPUT_PULLUP)
- NeoPixel: data33, power enable21
- I2C reserved: SDA3, SCL4 (for MPR121 later)
- Aux UART: RX38, TX39 (will be freed if needed for I2S)

## Done so far
- [x] FreeRTOS task structure (display, heartbeat, animation, menu, status)
- [x] Splash screen with company + version + provenance stub
- [x] Menu: Home → Music; Now Playing view with mock tracks and progress
- [x] Controls: 5-button input, long-press Select → Now Playing
- [x] Audio: I2S init; default volume 50%; 1 kHz tone play/pause
- [x] Audio: WAV and MP3 playback from SD to PCM5102A headphones; basic play/pause and volume via buttons
- [x] Input: MPR121 12‑pad wheel prototype; center‑of‑mass interpolation; anti‑drift gating; top‑right debug counter
- [x] NeoPixel feedback on button presses
- [x] SD (SPI) mount + directory listing; periodic card presence check
- [x] Versioning + build manifest script (firmware.json); signing plan in docs

## Next
- [ ] SD file browser UI (list /Music, select to play)
- [ ] Underrun detection and playback stress tests (long-run)
- [ ] Backlight dimming while playing; basic power optimization pass
- [ ] Help overlay (key/button map)
- [ ] OTA web skeleton + version/status page
- [ ] MPR121 wheel: finalize scroll UX (interrupt‑driven read, threshold matrix, smoothing)
- [ ] Design PCB wheel: 12‑segment ring, ground guard, PET overlay; short I2C; shield/keepout
- [ ] RP2040 USB coprocessor (breadboard):
  - Wire GND + link (prefer UART on free pins; avoid S3 GPIO38/39 due to I2S conflict; fallback I2C)
  - RP2040 TinyUSB composite (CDC+HID) example; echo framed commands
  - ESP32 bridge: send KEYSEQ over link; verify HID inject on host
  - Document pin map and minimal command set used
- [ ] Radio + antenna module (breadboard):
  - Option A: CC1101 (sub‑GHz) via SPI (shared bus, new CS); verify WHOAMI/RSSI, basic sniffer
  - Option B: nRF24L01+ via SPI (PA/LNA if available); channel scan + basic RX
- [ ] SDMMC 4-bit migration on PCB (throughput improvement)
 - [ ] Storage decision for Mini (microSD vs eMMC vs SPI NAND) and PCB footprint selection
 - [ ] ESP32‑PICO migration checklist: pins, SDMMC, I2S, SPI/TFT, I2C (MPR121), power/LDOS, RF/antenna keepouts
 - [ ] USB coprocessor (RP2040) — plan only now; implement in a later phase
 - [ ] Implement ESP32↔RP2040 UART protocol (Appendix) and a minimal HID demo

## Appendix: USB Coprocessor UART Command Spec (RP2040, deferred)

- Transport: UART 115200 8N1 (default), optional RTS/CTS; framing is binary with CRC.
- Frame format (little‑endian):
  - Preamble: 0xAA 0x55
  - Version: 0x01
  - Type: uint8
  - Length: uint16 (payload bytes)
  - Payload: [Length]
  - CRC16‑CCITT (0x1021, init 0xFFFF) over Version..Payload
- Types (ESP32→RP2040):
  - 0x01 PING
  - 0x10 HID_KEYSEQ: payload = sequence of events; each event 5 bytes:
    - evType uint8 (0=press,1=release,2=delay)
    - keycode uint8 (USB HID usage ID; ignored if delay)
    - modifiers uint8 (bitmask: L/R ctrl/shift/alt/gui; ignored if delay)
    - ms uint16 (delay or auto‑release time)
  - 0x11 HID_TEXT_UTF8: payload = UTF‑8 bytes (RP2040 maps to keycodes for selected layout)
  - 0x12 MOUSE_MOVE: dx int16, dy int16, wheel int8, buttons uint8 (bitmask)
  - 0x20 MSC_CONTROL: op uint8 (0=present,1=remove)
  - 0x30 STATUS_REQUEST
- Types (RP2040→ESP32):
  - 0x02 PONG
  - 0x31 STATUS_RESPONSE: fwVer uint16, caps uint16 (bit0=CDC, bit1=HID, bit2=MSC), uptimeMs uint32
  - 0x7E ACK, 0x7D NAK (code uint8)
  - 0x7F ERROR: code uint8, detailLen uint8, detail[]
- Handshake: RP2040 sends STATUS_RESPONSE on boot; ESP32 sends PING and expects PONG within 500 ms. Retry ×3 then degrade.
- Reliability: Every command expects ACK/NAK. Retransmit on timeout (300 ms) up to 3 times. Drop on CRC error.
- Layout: default keyboard layout = US; future 0x32 SET_LAYOUT to switch.
- Example (text): ESP32 sends HID_TEXT_UTF8("Hello\n"); RP2040 emits key events with auto‑release between chars.
