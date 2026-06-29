## Why

We need a consistent way to surface dynamic display values from devices and global firmware sources in text widgets. The existing display layout flow can store literal text and legacy bindings, but it does not give the designer a structured placeholder catalog or a stable UI workflow for inserting and validating metric references.

## What Changes

- Add a unified metric placeholder model with explicit namespaces for device, system, and wifi sources.
- Expose a placeholder catalog to the display designer so users can pick a device and metric instead of typing raw identifiers.
- Keep missing device metrics soft-failing at render time so layouts remain usable when a source device is deleted or temporarily unavailable.
- Add refresh interval handling for text widgets that use dynamic placeholders so the display can redraw on a bounded cadence.
- Surface placeholder validation and availability state in the designer without blocking save for soft-missing metrics.

## Capabilities

### New Capabilities
- `display-metric-placeholders`: Structured metric placeholder catalog, text widget placeholder resolution, and dynamic refresh behavior for display text widgets.

### Modified Capabilities
- `oled-display-layout-designer`: Update the OLED display designer contract to support structured placeholder selection, placeholder validation, and refresh interval controls for dynamic text widgets.

## Impact

- Portal SPA display designer workflows for SSD1306 and ST7735 layouts.
- Firmware display metric resolution, layout rendering cadence, and text evaluation.
- Portal API surface for metric placeholder catalog and value lookup.
- Unit tests and native display-renderer tests that now exercise realistic placeholder scenarios.
