## Context

The current portal frontend already models display widgets as typed layouts with bitmap import helpers and format-aware raster codecs. The firmware backend still persists a narrower display layout shape and the `st7735` frontend config is missing SPI bus fields, so the contract diverges at both the widget and device levels.

## Goals / Non-Goals

**Goals:**
- Use one shared display layout contract for `ssd1306` and `st7735`.
- Round trip typed widgets, bitmap data, and display-specific bitmap formats through the API.
- Add `st7735` SPI configuration to the frontend.
- Preserve existing `ssd1306` behavior where possible and keep old layouts readable.

**Non-Goals:**
- Build a full pixel editor or image management system.
- Redesign the display designer UX beyond what is needed for contract parity.
- Introduce new external image-processing dependencies.

## Decisions

### Keep one shared display layout codec
The backend should evolve the current shared display layout codec instead of creating separate per-device layout stores. This keeps `ssd1306` and `st7735` aligned and avoids duplicating binary persistence logic.

Alternative considered: separate codecs per display type. Rejected because the frontend already shares the same widget model and the contract mismatch is structural, not device-specific.

### Store bitmap payloads inside the display layout record
Bitmap widgets should persist their payload as part of the device-scoped display layout state so the portal and firmware keep one authoritative copy of the widget draft.

Alternative considered: a separate asset store. Rejected for this change because it adds cross-reference complexity and does not solve the contract mismatch.

### Keep SPI configuration in the `st7735` device form
`st7735` needs its own device form because the backend config includes SPI-specific fields that `ssd1306` does not use.

Alternative considered: reuse the `ssd1306` form with conditional branches. Rejected because the transport dependency is different and would make the form harder to reason about.

## Risks / Trade-offs

- [Risk] Bitmap payloads may exceed the current JSON/request budget. -> Keep bitmap validation bounded and align request/storage limits with the supported payload sizes.
- [Risk] Enlarging display payloads can increase response sizes. -> Use bounded document sizes and only include layout fields that are required for the editor.
- [Risk] Frontend/backend bitmap format drift. -> Keep `ssd1306` and `st7735` format profiles explicit and test round trips for both.

