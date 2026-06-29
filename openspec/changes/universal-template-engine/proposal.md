## Why

Display placeholder handling is currently coupled to the display designer and metric catalog logic, which makes preview behavior narrow and hard to reuse. The editor also shows raw `{{...}}` text in preview and sizing hints, so users cannot reliably judge the rendered output or space requirements. This change targets the display designer UI first so the preview and sizing experience become accurate there before any broader reuse.

## What Changes

- Introduce a generic template engine that can parse placeholders, extract placeholder names, apply filters, and render text from a plain object resolver.
- Keep template syntax independent from display and metric concepts so the engine can be reused later, but introduce it for the display designer UI first.
- Use the generic engine as the shared foundation for metric-backed display placeholders through a thin metric adapter in the designer UI.
- Update display designer preview and fit/auto-size calculations to render resolved sample values instead of raw placeholder source text when sample data is available.
- Preserve raw template source text in widget configs and runtime payloads; the new engine only affects parsing, rendering, and preview-time evaluation.

## Capabilities

### New Capabilities
- `universal-template-engine`: generic placeholder parsing, filter application, placeholder extraction, and object-based text rendering.

### Modified Capabilities
- `oled-display-layout-designer`: designer preview and sizing behavior use resolved placeholder sample values instead of raw placeholder text when sample data exists.

## Impact

- Frontend template parsing and rendering utilities in the portal SPA, scoped initially to the display designer UI.
- Display designer inspector, preview canvas, and text fit/auto-size logic.
- Metric-backed placeholder handling through an adapter layer rather than display-specific code.
- Unit and Playwright coverage for template parsing, filtering, placeholder extraction, and display preview behavior.
