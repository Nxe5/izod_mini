# Phase 04 — Third‑Party Apps Platform

Goal: Allow installing and running simple third‑party apps, managed via an app library.

## Scope
- App manifest + packaging
- App store/library UI
- Sandboxed APIs: display, input, storage, network
- App signing and provenance labels

## Features
- Browse/Install/Update/Remove apps
- Launchpad UI; per‑app settings
- App lifecycle: onInstall, onStart, onPause, onStop
- Signing: `app.json` + `app.sig`, verified with developer or third‑party public key
- Trust badges: Official / Third‑party / Unknown

## Acceptance Criteria
- Install an example app from SD or URL
- App runs without blocking system tasks
- Verified apps labeled Official; unverified labeled Third‑party or Unknown per policy

## Libraries & References
- Inspiration: [flipperzero-firmware](https://github.com/flipperdevices/flipperzero-firmware)
- ESP32 tools: filesystem APIs (SPIFFS/SD), HTTP client
- Signing: same Ed25519 verification as firmware

## Steps
1. Define app manifest (JSON) and packaging layout
2. Define minimal SDK (C++ bindings) + examples
3. Implement install/update from SD/URL with signature verification
4. Launchpad UI + trust badges; configurable policy (allow unverified?)
