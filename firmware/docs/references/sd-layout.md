# SD Card Layout & Usage

## Current Mode
- SPI mode (breadboard):
  - SCK=GPIO36, MOSI=GPIO35, MISO=GPIO37, CS=GPIO8, 3V3, GND
- SDMMC 4-bit planned for PCB (faster throughput)

## Folder Structure (auto-created)
- /Music
- /Playlists
- /Pentest (wifi, ble, lf_rfid, nfc, ir, subghz, nrf24, badusb, scripts, captures, dumps)
- /Apps
- /Firmware
- /System (thumbs/, library.json)

Create/ensure layout:
- Auto on first mount, or Serial 'F' to ensure, 'X' to quick-format and recreate

## Adding Music
- For now: WAV 44.1 kHz, 16-bit, stereo PCM
- Place files under /Music (e.g., /Music/Test/test.wav)
- Artwork: optional folder.jpg in each album folder (cached later)

## Playing Music (temporary serial controls)
- Open Serial Monitor @ 115200
- 'w' → play first WAV found under /Music
- 'x' → stop WAV
- 'p' or space → play/pause tone (use 'x' to stop WAV first)
- '+'/'-' → volume

## Status
- Every 8s: prints audio and SD states
- sd=OK requires card present and mountable

## Future
- SDMMC 4-bit
- Library indexing (/System/library.json)
- MP3/FLAC/OGG decoders
- SD-based file browser UI
