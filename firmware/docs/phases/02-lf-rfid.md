# Phase 02 — LF RFID (Read/Write/Emulate)

Goal: Add 125 kHz LF RFID for simple access control (e.g., gym door) with read, write (where legal), and emulate modes.

## Scope
- LF RFID reader/writer hardware (module or custom front-end)
- Protocols: EM4100/EM4102, HID Prox (where lawful)
- UI flows for read, save, emulate; tag library on SD

## Architecture
- Tasks
  - RFID Task (Core 0): sampling/decoding; low jitter
  - UI Task: mode selection, results
  - Storage Task: save/load tag dumps to SD
- Files
  - /RFID/Tags/*.bin or *.txt; include metadata JSON

## Features
- Read Tag → show format, ID; save to SD
- Emulate Tag → select from library; transmit
- Write Tag (if supported by tag type/module)
- Quick actions from Now Playing (long‑press to emulate favorite)

## Acceptance Criteria
- Reliable read of supported LF tags at typical distances
- Emulation success with test readers (lab bench)
- Saved tags re‑loadable and emulatable

## Libraries & References
- Inspiration/workflows: [flipperzero-firmware](https://github.com/flipperdevices/flipperzero-firmware)
- ESP32 pentest ideas: [ESP32-DIV](https://github.com/cifertech/ESP32-DIV)

## Implementation Steps
1. Select LF front-end and define pinout
2. Implement sampling + decoder for baseline formats
3. Build UI: read → details → save, and emulate picker
4. Storage format for dumps + metadata
5. Add safety/legal prompts; disable risky ops by default

## Notes
- Observe local laws; provide clear disclaimers and safeguards
- Keep LF processing off UI core; avoid audio dropouts
