## Why

The frontend already supports typed display widgets, bitmap import, and an `st7735` display model, but the firmware-side display layout codec and API adapters still serialize an older OLED-specific shape. That leaves the portal and firmware out of sync for bitmap widgets, color formats, and SPI-based display configuration.

## What Changes

- Extend the shared display layout contract so widget `type`, `bitmapData`, `bitmapFormat`, and `keepAspectRatio` round trip through the API and persisted state.
- Update firmware display layout persistence so `ssd1306` and `st7735` both use the shared typed layout codec.
- Add `st7735` SPI bus configuration UI on the frontend, including `spiBusDeviceId` and `chipSelectPin`.
- Keep `ssd1306` monochrome bitmap handling and `st7735` RGB565 bitmap handling distinct but governed by the same shared widget model.
- Preserve legacy `ssd1306` layouts while rejecting unsupported widget types or payloads that exceed the configured bounds.

## Capabilities

### New Capabilities
- `display-layout-parity`: Shared display widget, bitmap, and device configuration parity for frontend and firmware.

### Modified Capabilities
- `oled-display`: Extend the display layout data model and persistence contract with typed widgets and bitmap payloads.
- `oled-display-layout-designer`: Keep the designer aligned with the typed bitmap widget contract.
- `portal-api-controllers`: Update device REST serialization/parsing for display layouts and persisted state.
- `portal-web-app`: Add `st7735` SPI configuration UI and keep display widget models aligned with firmware.
- `device-type-catalog`: Ensure display device metadata stays consistent with the new typed display contract.

## Impact

- Firmware display layout codec and device API adapters.
- Device registry persisted-state flow for display layouts.
- Portal SPA display device models, forms, and detail surfaces.
- Portal SPA layout designer and bitmap import code paths.
- OpenSpec specs and tests for display layout parity.

