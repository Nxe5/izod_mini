# iZod Classic Plan (Classic‑style)

Comprehensive checklist for the larger variant with additional radios: CC1101, IR, nRF24, in addition to all Mini capabilities.

## 0) Foundations
- [ ] Shared codebase with Mini; build flags select Classic
- [ ] Power budget and regulators sized for radios
- [ ] Mechanical plan for modules and antennas

## 1) Carry‑over from Mini
- [ ] Display/UI complete
- [ ] Wheel/inputs complete
- [ ] Audio pipeline complete
- [ ] SD/MMC 4‑bit and indexer complete
- [ ] Web/OTA complete
- [ ] Apps platform v1

## 1b) Core add‑ons (Classic)
- [ ] Bluetooth Classic audio module (A2DP) integration
  - Module selection (e.g., QCC/CSR) + I2S/UART wiring
  - A2DP sink to PCM5102; controls passthrough in UI
- [ ] 125 kHz LF RFID: EM4095 front‑end + tuned coil
  - Read EM4100/HID; Write T5577; Emulate from SD
  - UI flows: read → save → emulate; write assistant

## 2) CC1101 (Sub‑GHz)
- [ ] SPI bring‑up and frequency plan (433/868/915)
- [ ] RX: capture and visualize packets
- [ ] Replay workflows (legal, lab only)
- [ ] Protocol experiments (modulation, data rates)

## 3) IR Blaster/Receiver
- [ ] IR TX/RX wiring and diode driver
- [ ] Decode NEC/RC5/etc.; transmit
- [ ] UI: capture → name → send

## 4) nRF24
- [ ] Bring‑up (addresses, pipes)
- [ ] Scanners and simple sniffing
- [ ] Demo integrations (RC ideas)

## 5) Integration UX
- [ ] Radio hub screen; per‑radio apps
- [ ] Logging to SD; export flows
- [ ] Safety prompts and legal notices

## 6) QA & Field Tests
- [ ] Coexistence tests (audio + radios)
- [ ] Battery life with radios active
- [ ] Thermal and EMC sanity

## Release & Signing
- [ ] Carry over Mini signing: manifest + signature
- [ ] Public key trust store; potential multiple maintainers
- [ ] UI provenance in app/radio tools
- [ ] Key rotation procedure documented

## References
- ESP32-DIV: https://github.com/cifertech/ESP32-DIV
- nRFBox: https://github.com/cifertech/nRFBox
- EvilDuck: https://github.com/cifertech/EvilDuck
- nRF24 jammer: https://github.com/hugorezende/nRF24L01-WiFi-Jammer
- Universal RC: https://github.com/alexbeliaev/Universal-RC-system/tree/master
