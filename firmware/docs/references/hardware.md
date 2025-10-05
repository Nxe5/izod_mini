# Hardware References

This page tracks the exact hardware we’re using and the prototype pin maps.

## MCU
- Adafruit ESP32-S3 Feather (v1) — Product 5477
  - Product page: https://www.adafruit.com/product/5477
  - Learn guide: https://learn.adafruit.com/adafruit-esp32-s3-feather

## Display
- ST7789 TFT (240×280) over SPI
- Prototype pins (custom wiring we use now):
  - CS = GPIO12
  - DC = GPIO13
  - SCK = GPIO36
  - MOSI = GPIO35
  - RST = not connected (−1)
  - BL  = optional (GPIO6)

## Audio
- PCM5102 I2S DAC
- Prototype pins:
  - I2S BCLK = GPIO1
  - I2S WS/LRCK = GPIO2
  - I2S DATA = GPIO42

## Storage
- SD card (SDMMC 4‑bit planned; single‑bit minimal wiring for breadboard)
- Current prototype pins (single‑bit minimal used only for bring‑up):
  - CMD = GPIO38
  - CLK = GPIO47
  - D0  = GPIO14
  - (D1/D2/D3 to be added on PCB for 4‑bit)

## TouchWheel0 (development‑only)
- Capacitive wheel prototype using 3 analog channels
- Feather pins for ADC during breadboard bring‑up:
  - GPIO 5, GPIO 6, GPIO 9
- Notes:
  - Keep these three pins clear of other functions
  - Later this will be replaced by MPR121 wheel; this 3‑pad setup is for dev only

## Power & Misc
- 3.3 V logic; power the display at 3.3 V
- Use common ground across modules
- Keep SPI lines short for the TFT on breadboard; add series resistors if needed for signal integrity

## Pin Use Summary (Prototype)
- Display (ST7789): CS12, DC13, SCK36, MOSI35, RST−1, BL6
- Audio (PCM5102): BCLK1, WS2, DATA42
- I2C (for future MPR121): SDA3, SCL4
- SD (proto): CMD38, CLK47, D0 14
- TouchWheel0 (dev): A0, A1, A2

Refer to the Learn guide pinout for the Feather to confirm ADC mappings and available pins for your specific board revision.
