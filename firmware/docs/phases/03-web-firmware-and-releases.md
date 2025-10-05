# Phase 03 — Web Interface, OTA, and GitHub Releases

Goal: Simple web UI for settings and firmware management; OTA updates sourced from GitHub Releases.

## Scope
- Local web UI (Async) for status, settings, logs
- OTA update mechanism
- Release pipeline using GitHub Releases artifacts
- Signed firmware manifest and verification (provenance)

See `docs/references/signing.md` for reusable signing procedures.

## Features
- Web UI
  - Device info (version, storage, Wi‑Fi)
  - Settings: Wi‑Fi, display, audio, inputs
  - Log viewer, file browser (SD)
- OTA
  - Local file upload
  - Fetch latest release from GitHub
  - Rollback on failure
- Signing & Provenance
  - `firmware.json` generated at build (name, version, date, commit, env, binary)
  - `firmware.sig` detached signature (Ed25519 recommended)
  - Device stores developer public key; verifies manifest and/or binary before install
  - UI badge on splash and About: Official / Community signed / Community build

## Acceptance Criteria
- Update succeeds from web UI without serial cable
- Versioning shown in UI; persistent across reboots
- Signature verification blocks tampered/unsigned artifacts (unless user overrides)

## Libraries & References
- Async web: ESPAsyncWebServer, AsyncTCP
- OTA: ArduinoOTA, Update.h
- Releases workflow: GitHub Actions + Releases
- Signing: libsodium or tiny-Ed25519 for verification on device

## Steps
1. Async web server skeleton + auth
2. Build settings pages + JSON API
3. Implement OTA endpoints (upload + URL fetch)
4. GitHub workflow to attach firmware.bin + firmware.json + firmware.sig to Releases
5. Device-side verification (Ed25519 verify) before Update.write()
6. UI provenance indicator on splash/About and override path (if allowed)
