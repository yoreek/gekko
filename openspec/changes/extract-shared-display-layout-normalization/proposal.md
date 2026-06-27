## Why

The portal display layout code now has shared display concepts, but the actual layout normalization still lives in the SSD1306 module. This makes the next display controller inherit either duplicated code or incomplete layout handling.

## What Changes

- Add a shared display layout profile contract for controller-specific defaults and limits.
- Move generic layout/widget normalization, encoding, and change comparison out of SSD1306-specific files.
- Keep the existing SSD1306 public model API as thin wrappers so current components and tests do not need broad rewrites.
- Update the ST7735 layout model to reuse the same shared normalization path without implementing the ST7735 designer or firmware runtime.
- Add focused unit coverage for shared normalization behavior and the SSD1306/ST7735 wrappers.
- Do not change firmware device type names, REST contracts, binary layout persistence, or live display rendering in this change.

## Capabilities

### New Capabilities
- `display-layout-normalization`: Shared portal-side display layout profile and normalization behavior for display controller models.

### Modified Capabilities

## Impact

- Affected frontend model files under `portal-spa/src/models/devices/display/`, `portal-spa/src/models/devices/ssd1306/`, and `portal-spa/src/models/devices/st7735/`.
- Focused tests under `portal-spa/tests/unit/models/devices/`.
- No new runtime dependency.
- No firmware, REST API, storage, or device registry changes.
