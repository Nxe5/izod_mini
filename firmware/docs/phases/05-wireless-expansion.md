# Phase 05 — Wireless Expansion (CC1101, IR, nRF24)

Goal: Add modular radio capabilities to the Classic variant.

## Scope
- CC1101 sub‑GHz (433/868/915 MHz)
- IR blaster + receiver
- nRF24L01 2.4 GHz

## Features
- CC1101: RX/record, replay where lawful; protocol experiments
- IR: record/transmit common protocols (NEC/RC5, etc.)
- nRF24: scanners, jammers (lab only), RC integrations

## Acceptance Criteria
- Receive + replay demo flows in lab
- IR control of a consumer device

## Libraries & References
- CC1101: ELECHOUSE CC1101 lib; 
  - Inspirations: [ESP32-DIV](https://github.com/cifertech/ESP32-DIV), [EvilDuck](https://github.com/cifertech/EvilDuck)
- nRF24: RF24 + examples; inspirations
  - [nRFBox](https://github.com/cifertech/nRFBox)
  - [nRF24L01-WiFi-Jammer](https://github.com/hugorezende/nRF24L01-WiFi-Jammer)
  - "Poor man’s 2.4 GHz scanner" (Arduino Forum): [thread](https://forum.arduino.cc/t/poor-mans-2-4-ghz-scanner/54846)
  - Universal RC ideas: [Universal-RC-system](https://github.com/alexbeliaev/Universal-RC-system/tree/master)
- IR: IRremoteESP8266

## Steps
1. Break out headers for radios; define power budget
2. Bring up each radio with loopback demos
3. Integrate into UI with proper disclaimers and safe defaults
