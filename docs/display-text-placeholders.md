# Display Text Placeholders

This document defines how metric placeholders work in display text widgets across the frontend, firmware, and mock/runtime layers.

## Scope

- Applies to text widgets on SSD1306 and ST7735 display layouts.
- Placeholder strings live directly in `widget.text`.
- Placeholders are evaluated at render time, not stored as a separate field.
- The same text may contain plain text and any number of placeholders.

## Placeholder Syntax

Supported forms:

- `{{dev.<deviceId>.<metricKey>}}`
- `{{system.<metricKey>}}`
- `{{wifi.<metricKey>}}`

Examples:

- `Room {{dev.670845748.status}}`
- `IP {{wifi.station_ip}}`
- `Boot {{system.time}} / {{system.uptime}}`

Whitespace inside the braces is tolerated by the parsers.

## Frontend Behavior

The frontend parses and validates placeholders in `portal-spa/src/models/metrics/placeholders.ts`.

Rules:

- Any number of placeholders is allowed in the same text field.
- Valid placeholders are inserted through the placeholder builder UI.
- Manually typed placeholders are validated on every edit.
- A text widget with no placeholder is treated as static text.
- Invalid placeholder syntax is reported, but it does not block preview rendering.
- A placeholder that exists syntactically but is not present in the fetched catalog is treated as unavailable.

Preview behavior:

- Available placeholders are replaced with their `preview` value in the designer preview.
- Unavailable placeholders remain in the preview text until the runtime resolves them.
- Multiple placeholders are replaced independently.

## Firmware Behavior

The firmware runtime evaluates display text in `src/devices/display/DisplayTextEvaluator.cpp`.

Rules:

- Multiple placeholders are supported.
- Each placeholder is parsed independently.
- If a placeholder cannot be parsed or cannot be resolved, it is replaced with an empty string.
- A bad placeholder does not cancel the full text render.
- Static text still renders unchanged.
- Metric-bound widgets without inline placeholders continue to resolve through `bindingKind`, `metricNamespace`, `sourceDeviceId`, and `metricId`.

## Validation Semantics

The current frontend validation statuses are:

- `static`
- `valid`
- `unavailable`
- `invalid`

The old single-placeholder `multiple` restriction was removed.

The firmware evaluator still tracks render outcomes such as:

- `Static`
- `Resolved`
- `MissingMetric`
- `InvalidPlaceholder`
- `InvalidWidget`
- `Truncated`

These statuses are diagnostic and should not prevent the renderer from drawing the text unless the widget itself is invalid or the output is truncated.

## Metric Catalog

The frontend gets placeholder metadata from `/api/metrics/placeholders`.

In mock mode, the catalog is synthesized in `portal-spa/src/mock/handlers.ts` from device runtime state and global runtime values.

The catalog carries:

- `placeholder`
- `namespace`
- `sourceId`
- `metricId`
- `metricKey`
- `label`
- `valueType`
- `available`
- `preview`

## Saving Layouts

Saving a display layout does not serialize placeholders separately.

The saved widget payload should include:

- `text`
- `bindingKind`
- `metricNamespace`
- `sourceDeviceId`
- `metricId`
- `refreshIntervalMs`

Layout normalization keeps those fields consistent before persistence.

## Notes

- Long placeholder text should use a multiline text editor in the UI.
- Text widget width and height still define the render box.
- Placeholder evaluation happens after layout loading and before widget drawing.
- The rendered output may be partially empty if some placeholders are unavailable.

## Related Docs

- [OLED text rendering notes](./oled-text-rendering-notes.md)
- [OLED display layout persistence](./oled-display-layout.md)
- [REST API contract](./rest-api-contract.md)
