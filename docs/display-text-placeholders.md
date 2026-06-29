# Display Text Placeholders

This document defines how metric placeholders work in display text widgets across the frontend, firmware, and mock/runtime layers.

## Scope

- Applies to text widgets on SSD1306 and ST7735 display layouts.
- Placeholder strings live directly in `widget.text`.
- Placeholders are evaluated at render time, not stored as a separate field.
- The same text may contain plain text and any number of placeholders.
- The placeholder pipeline keeps the raw widget text as the source of truth and compiles a transient runtime AST close to render time.

## Value Model

Placeholder resolution is typed before it becomes text.

- `MetricValue` is the transport object between metric sources and placeholder formatting.
- The value model is intentionally small: `Null`, `Bool`, `Int`, `Float`, and `String`.
- Numeric metrics stay numeric until the value formatter turns them into display text.
- String filters are a final formatting step; they do not replace typed metric resolution.
- The renderer should not infer numeric meaning from a preformatted string when a typed source value exists.

## Placeholder Syntax

Supported forms:

- `{{dev.<deviceId>.<metricKey>}}`
- `{{system.<metricKey>}}`
- `{{system.wifi.<metricKey>}}`
- Optional filters may follow the placeholder body with `|`, for example `{{system.wifi.station_ip | upper}}`.

Examples:

- `Room {{dev.670845748.temperature}}`
- `State {{dev.670845748.state}}`
- `IP {{system.wifi.station_ip}}`
- `Boot {{system.time}} / {{system.uptime}}`
- `Device {{dev.123.temperature | upper}}`

Whitespace inside the braces is tolerated by the parsers.
Whitespace around the `|` separator is also tolerated.

## Frontend Behavior

The frontend parses and validates placeholders in `portal-spa/src/models/metrics/placeholders.ts`.

Rules:

- Any number of placeholders is allowed in the same text field.
- Valid placeholders are inserted through the placeholder builder UI.
- Manually typed placeholders are validated on every edit.
- A text widget with no placeholder is treated as static text.
- Invalid placeholder syntax is reported, but it does not block preview rendering.
- A placeholder that exists syntactically but is not present in the fetched catalog is treated as unavailable.
- The SPA parser mirrors the firmware grammar, including trailing filters.

Preview behavior:

- Available placeholders are replaced with their `preview` value in the designer preview.
- Unavailable placeholders remain in the preview text until the runtime resolves them.
- Multiple placeholders are replaced independently.
- Filters are applied to the preview value after placeholder resolution.

## Firmware Behavior

The firmware runtime evaluates display text in `src/devices/display/DisplayTextEvaluator.cpp`.

Rules:

- Multiple placeholders are supported.
- Each placeholder is parsed independently.
- If a placeholder cannot be parsed or cannot be resolved, it is replaced with an empty string.
- A bad placeholder does not cancel the full text render.
- Static text still renders unchanged.
- Metric-bound widgets without inline placeholders continue to resolve through `bindingKind`, `metricNamespace`, `sourceDeviceId`, and `metricId`.
- Device placeholder source devices are recorded separately as `metric_source` registry dependencies so delete protection can see them.
- Value formatting happens before final text layout, so the renderer receives typed metric output plus optional placeholder filters, not a raw device-specific sensor object.

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

The catalog describes placeholder candidates for editing only. It is not the runtime value contract.

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

The runtime AST and any compiled placeholder references are transient and are rebuilt from `text` after layout load.

## Notes

- Long placeholder text should use a multiline text editor in the UI.
- Text widget width and height still define the render box.
- Placeholder evaluation happens after layout loading and before widget drawing.
- The rendered output may be partially empty if some placeholders are unavailable.
- String filters are intentionally small and bounded. New filters should be added only when they map cleanly to typed metric values or existing display formatting rules.

## Related Docs

- [OLED text rendering notes](./oled-text-rendering-notes.md)
- [OLED display layout persistence](./oled-display-layout.md)
- [REST API contract](./rest-api-contract.md)
