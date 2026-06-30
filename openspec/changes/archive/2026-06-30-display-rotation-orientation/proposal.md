## Why

Display orientation is currently implicit and inconsistent between device types. The designer should not force users to reason about upside-down 90/270 degree rotations, while the firmware still needs the full hardware rotation range for real mounting and rendering.

The current ST7735 and SSD1306 setups also appear to assume opposite physical orientations. That makes preview, layout bounds, and runtime output diverge unless orientation becomes an explicit part of the device contract.

## What Changes

- Add a display orientation contract that separates human-facing designer orientation from hardware rotation.
- Treat hardware rotation values `0..3` as the firmware/runtime source of truth.
- Treat designer orientation as a simplified logical model where `0/2` and `1/3` are equivalent groups for layout work.
- Make display previews and layout bounds follow the effective orientation so canvas width/height and widget geometry match the real panel orientation.
- Correct the default orientation assumptions for ST7735 and SSD1306 so each display type opens in the expected physical orientation.
- Persist orientation as part of device configuration and migrate existing devices safely.

## Capabilities

### New Capabilities
- `display-orientation`: shared display orientation rules for designer grouping, firmware rotation, preview geometry, and per-display defaults.

### Modified Capabilities
- `oled-display-layout-designer`: designer orientation, preview, and geometry rules use the new display orientation contract.
- `oled-display`: firmware runtime applies persisted display rotation and respects the new orientation contract.
- `device-configuration`: device config schema includes persisted display orientation / rotation fields and migration behavior.

## Impact

- Portal SPA display designer panels, preview widgets, and device forms.
- Firmware display runtime initialization for SSD1306 and ST7735.
- Device config schema versioning and migration for display devices.
- Existing saved layouts and device configs need backward-compatible defaults so current devices keep rendering correctly after the change.
