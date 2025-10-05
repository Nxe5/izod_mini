# Phase 10 — Roadmap: Next Displays & Migration

Goal: Prepare for future display hardware and potential migration of subsystems to ESP‑IDF where beneficial.

## Scope
- Alternate displays (higher res, different controllers)
- Evaluate migration of audio/SD to ESP‑IDF components for performance

## Steps
1. Abstract display driver behind clean interface
2. Add a second display backend behind a build flag
3. Prototype ESP‑IDF audio path while UI stays Arduino
4. Measure gains, decide per‑module migration

## Notes
- Keep code modular (DisplayManager, AudioManager, InputManager)
- Maintain Arduino + FreeRTOS developer experience
