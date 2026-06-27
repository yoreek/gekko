## 1. Shared Model Foundation

- [x] 1.1 Add `portal-spa/src/models/devices/display/profile.ts` with `DisplayLayoutProfile` and SSD1306/ST7735 profile constants for current defaults and limits.
- [x] 1.2 Add `portal-spa/src/models/devices/display/layout-normalizer.ts` with shared default layout, default widget, widget normalization, layout normalization, encoding, and change comparison helpers.
- [x] 1.3 Keep generic normalization helpers independent from Vue component paths and avoid importing from `components/devices/display/...` in the shared model layer.

## 2. Controller Wrappers

- [x] 2.1 Refactor `portal-spa/src/models/devices/ssd1306/layout.ts` so existing exported SSD1306 helpers delegate to the shared normalizer through the SSD1306 profile.
- [x] 2.2 Preserve the existing SSD1306 encoded layout shape, bitmap defaults, `mono1` format, page limits, widget limits, and `ssd1306LayoutChanged(...)` behavior.
- [x] 2.3 Refactor `portal-spa/src/models/devices/st7735/layout.ts` so it uses the shared normalizer through the ST7735 profile and preserves normalized pages/widgets.
- [x] 2.4 Do not add ST7735 designer components, ST7735 form/detail registry entries, firmware changes, or REST contract changes in this change.

## 3. Tests

- [x] 3.1 Add focused unit tests for shared display layout normalization limits, widget defaults, bitmap payload validation, and encoding shape.
- [x] 3.2 Update SSD1306 layout tests to prove the public SSD1306 wrapper behavior did not change.
- [x] 3.3 Add ST7735 layout model tests proving normalized pages/widgets are preserved and RGB565 bitmap defaults are applied.

## 4. Verification

- [x] 4.1 Run the focused portal unit tests for display layout models.
- [x] 4.2 Run the broader portal unit test command if focused tests pass.
- [x] 4.3 Review the diff to confirm no firmware, REST adapter, device registry, or designer component rewrite was included.
