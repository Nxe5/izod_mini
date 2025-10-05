# Phase 07 — Storage (SDMMC 4‑bit) & Library Indexing

Goal: Fast SD access in 4‑bit mode, robust library indexing, and metadata caching.

## Scope
- SDMMC 4‑bit mode wiring + driver config
- Library scan + cache (artist/album/track)
- Metadata (ID3/FLAC) parsing

## Acceptance Criteria
- Mount SD in 4‑bit mode; sustained reads for audio + UI
- Library cache reduces cold boot scan time significantly

## Libraries & References
- SD_MMC (Arduino) or ESP‑IDF sdmmc via Arduino
- Tag parsing libs for ID3/FLAC (or lightweight custom)

## Steps
1. Wire SD for 4‑bit; confirm signal integrity
2. Implement mount + throughput test
3. Build indexer + JSON cache on SD
4. Artwork strategy (folder.jpg, embedded later)
