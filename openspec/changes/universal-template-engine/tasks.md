## 1. Template Engine

- [ ] 1.1 Add a pure template engine module that tokenizes `{{name}}` placeholders, extracts placeholder metadata, validates filters, and renders from a flat resolver object.
- [ ] 1.2 Implement the built-in filter set (`text`, `upper`, `lower`, `trim`) in the shared engine with deterministic behavior.
- [ ] 1.3 Add focused unit tests for parsing, extraction, rendering, missing-value fallback, and invalid syntax/filter rejection.

## 2. Metric Adapter

- [ ] 2.1 Refactor the metric placeholder helper to build on the generic template engine instead of owning parsing and filter logic itself.
- [ ] 2.2 Add a metric-to-resolver adapter that maps metric catalog preview values into template engine inputs without exposing display-specific concepts.
- [ ] 2.3 Update existing metric placeholder unit tests to cover the new engine-backed behavior and preserve current validation semantics.

## 3. Designer Integration

- [ ] 3.1 Update the display designer inspector to resolve preview text from the metric catalog sample values before preview rendering and fit measurement.
- [ ] 3.2 Thread resolved preview text into OLED and ST7735 widget preview rendering while keeping the raw template text in the widget editor.
- [ ] 3.3 Update text auto-size and fit calculations to measure resolved sample text, including filtered placeholder output.
- [ ] 3.4 Preserve the current save-time validation behavior so malformed placeholders still block save and unresolved sample values still keep the draft editable.

## 4. Verification

- [ ] 4.1 Extend display designer Playwright coverage to verify that placeholder samples render in preview and filters affect the shown output.
- [ ] 4.2 Run the relevant unit test suite for template parsing, metric placeholder handling, and OLED text sizing.
- [ ] 4.3 Run the portal SPA checks needed to confirm the refactor does not break existing display designer flows.
