# Signing & Provenance Reference (Reusable)

Use this across projects to sign firmware and apps and show provenance badges.

## Key Policy
- Use an organization release key with yearly rotation: e.g. `ocho-labs-release-2025`.
- Embed only the PUBLIC key in firmware; keep PRIVATE key offline.
- For multiple projects, prefer the same org key per year rather than one key forever.
- Support a trust store with multiple pubkeys to allow rotation/transition.

## Generate Ed25519 Keys (OpenSSL)
```bash
# Private key (keep offline)
openssl genpkey -algorithm ed25519 -out ocho-labs-release-2025.key
# Public key (embed in firmware)
openssl pkey -in ocho-labs-release-2025.key -pubout -out ocho-labs-release-2025.pub
```

## Sign Artifacts
Recommended artifacts per release:
- `firmware.bin` (binary)
- `firmware.json` (manifest)
- `firmware.sig` (detached signature over `firmware.bin`)

Commands:
```bash
# Sign binary (Ed25519 raw)
openssl pkeyutl -sign -inkey ocho-labs-release-2025.key -in firmware.bin -out firmware.sig

# Verify locally
openssl pkeyutl -verify -pubin -inkey ocho-labs-release-2025.pub -sigfile firmware.sig -in firmware.bin
```

Optional: also sign `firmware.json` and attach `firmware.json.sig`.

## On-Device Verification
- Embed public key (32 bytes) in a trust store (e.g., `include/trust_store.h`).
- Verify Ed25519 detached signature before installing OTA.
- Derive badge:
  - Official (valid signature by Ocho Labs key)
  - Community signed (valid signature by other trusted key)
  - Community build (no/invalid signature)

## CI Integration
- Store `ocho-labs-release-2025.key` as a CI secret.
- After PlatformIO build, sign `firmware.bin`, upload `firmware.bin`, `firmware.json`, `firmware.sig` to GitHub Release.

## Rotation
- Add a new pubkey to trust store; ship update.
- Mark old key as deprecated in UI/docs.
- Remove old key after a defined grace period.

## Reuse Across Projects
- Reuse the same org/year key (e.g., `ocho-labs-release-2025`) for multiple projects.
- Each project still ships its own `firmware.json` and badges.
