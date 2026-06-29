## Why

Text widgets currently resolve placeholders by reparsing raw text during rendering, which makes the hot path more expensive than it needs to be. The current contract also only validates placeholder syntax loosely, so malformed placeholders, missing devices, and missing metrics are not handled as a first-class structured result for the designer or runtime.

## What Changes

- Add structured placeholder parsing for text widgets that produces a cached representation of literal text and placeholder references instead of scanning the raw string on every render.
- Validate placeholder syntax, referenced device existence, and referenced metric existence as part of the text-widget editing and save flow.
- Allow any number of placeholders in a single text widget, with invalid or unresolved placeholders treated as empty substitutions rather than aborting the entire render.
- Keep the persisted layout text as the source of truth while storing a compiled runtime form or equivalent cached parse result for rendering.
- Update the runtime renderer to consume the compiled placeholder structure so display refresh work stays bounded.
- Extend display documentation to describe multi-placeholder text widgets, validation behavior, and the compiled placeholder model.

## Capabilities

### New Capabilities
- `display-text-placeholder-ast`: Structured parsing and cached runtime compilation of text-widget placeholders for display rendering.

### Modified Capabilities
- `display-metric-placeholders`: Text widgets may contain multiple placeholders, and invalid or missing placeholder references are validated and soft-failed per placeholder instead of invalidating the whole render.
- `oled-display-layout-designer`: The designer validates text-widget placeholders against available devices and metrics and supports multiple placeholders per text widget.
- `oled-display`: Runtime layout handling compiles placeholder text into a cached representation used by render-time evaluation.

## Impact

- Firmware text-widget parsing, validation, and render paths for SSD1306 and ST7735 displays.
- Placeholder catalog and text widget inspector behavior in the portal SPA.
- Layout/documentation updates for display text widgets and placeholder handling.
- Tests for placeholder parsing, validation, and render-time fallback behavior.
