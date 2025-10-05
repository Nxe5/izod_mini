# Phase 06 — UI/UX Design

Goal: Deliver a clean iPod-like interface with split view and fluid navigation; evaluate LVGL later.

## Scope
- Split view: left list (text), right artwork/video (placeholder initially)
- Theming, fonts, icons
- Animations (selection highlight, progress)

## Acceptance Criteria
- Consistent 20–30 FPS with audio active
- Minimal redraw (dirty rectangles)

## Libraries & References
- Current stack: Adafruit_GFX + Adafruit_ST7789
- Future option: LVGL; inspiration: [tangara-fw](https://github.com/Gigahawk/tangara-fw)

## Steps
1. UI components: list, status bar, artwork pane
2. Navigation model: stack-based views
3. Asset pipeline: fonts/images on SD
4. Evaluate LVGL once Phase 01 stabilizes
