## Why

The firmware now supports two display backends with different buses and lifecycles, but the existing OLED implementation is still named and structured like a single-device-only path. That makes the SSD1306 and ST7735 backends harder to evolve independently and keeps the backend contract tied to a legacy `oled_display` name.

## What Changes

- **BREAKING** Rename the OLED backend contract from the legacy `oled_display` naming to the canonical `ssd1306` device model.
- **BREAKING** Remove the old OLED compatibility aliases and internal `src/devices/display/oled/` backend folder.
- Add a new SPI-backed `st7735` display backend alongside SSD1306.
- Introduce a shared display layout base so SSD1306 and ST7735 reuse the same layout normalization, persistence, and API wiring.
- Register both backends in the device registry, setup transfer codec, and REST API adapter registry.
- Update the backend test coverage to use the new canonical display model names and paths.

## Capabilities

### New Capabilities
- `st7735-display`: SPI-backed display device backend with its own config, REST adapter, and device registry support.

### Modified Capabilities
- `oled-display`: The SSD1306 display contract now uses the canonical `ssd1306` name and shared display layout base, replacing the legacy `oled_display` alias and backend folder.

## Impact

- Backend device model code under `src/devices/display/`.
- Device type registration and transfer codecs under `src/devices/core/` and `src/devices/registry/`.
- REST adapter registration under `src/integrations/common/` and `src/integrations/rest/`.
- Display layout persistence in device-scoped storage.
- Display-related native tests and backend naming.
