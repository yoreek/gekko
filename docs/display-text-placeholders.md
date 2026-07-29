# Display Text Placeholders

> User-facing guide: <https://yoreek.github.io/gekko/guides/displays/> — keep the two in sync when changing behavior described here.

This document defines how metric placeholders work in display widgets with text
content across the frontend, firmware, and mock/runtime layers.

## Scope

- Applies to Text widgets on SSD1306/ST7735, Character widgets on
  LCD1602/LCD2004, and Digital widgets on TM1637 display layouts.
- Placeholder strings live directly in `widget.text`.
- Placeholders are evaluated at render time, not stored as a separate field.
- The same text may contain plain text and any number of placeholders.
- The placeholder pipeline keeps the raw widget text as the source of truth and compiles a transient runtime AST close to render time.

## Value Model

Placeholder resolution is typed before it becomes text.

- `MetricValue` is the transport object between metric sources and placeholder formatting.
- The value model is intentionally small: `Null`, `Bool`, `Int`, `Float`, `String`, `DateTime`, and `Duration`.
- `DateTime` carries a real calendar timestamp (a `src/time/DateTime.h` object, timezone included) - `system.time` is the only metric that produces one today, via `DateTime::current()`.
- `Duration` carries an elapsed millisecond count with no calendar meaning - `system.uptime` produces one, from `millis()` since boot.
- Numeric metrics stay numeric until the value formatter turns them into display text.
- String filters are a final formatting step; they do not replace typed metric resolution.
- The renderer should not infer numeric meaning from a preformatted string when a typed source value exists.

## Placeholder Syntax

Supported forms:

- `{{dev.<deviceId>.<metricKey>}}`
- `{{system.<metricKey>}}`
- `{{system.wifi.<metricKey>}}`
- Optional filters may follow the placeholder body with `|`, for example `{{system.wifi.station_ip | upper}}`.
- A filter may take an argument with `name:arg`, for example `{{system.time | format:HH:mm:ss}}`.

Examples:

- `Room {{dev.670845748.temperature}}`
- `State {{dev.670845748.state}}`
- `IP {{system.wifi.station_ip}}`
- `Now {{system.time}}` (defaults to `HH:mm:ss`, local wall-clock time)
- `Up {{system.uptime}}` (defaults to `H:MM:SS`/`M:SS` elapsed since boot)
- `Device {{dev.123.temperature | upper}}`
- `Day {{system.time | format:EEEE}}`
- `Temp {{dev.123.temperature | fixed:1}}`

Whitespace inside the braces is tolerated by the parsers.
Whitespace around the `|` separator is also tolerated.

## Filter Reference

| Filter | Argument | Applies to | Notes |
|---|---|---|---|
| `text` | none | any | No-op; explicit "no filter". |
| `upper` | none | any | Uppercases the resolved text. |
| `lower` | none | any | Lowercases the resolved text. |
| `trim` | none | any | Trims leading/trailing whitespace from the resolved text. |
| `format` | pattern, required | `DateTime`, `Duration` | Token-substitution pattern (see below). Any other value type fails soft to empty, same as a missing metric. |
| `fixed` | digit count `0`-`6`, required | `Int`, `Float`, `Duration` | `toFixed()`-style decimal formatting. Any other value type fails soft to empty. |

`format` tokens (longest match wins): `YYYY`/`YY` (year), `MM`/`M` (month), `DD`/`D` (day), `HH`/`H` (hour - hour-of-day 0-23 for `DateTime`, **total elapsed hours** for `Duration`), `mm`/`m` (minute), `ss`/`s` (second), `EEEE`/`EEE` (full/short weekday name, `DateTime` only). Text inside `[square brackets]` is copied literally and never tokenized - use it for any literal letter that would otherwise be read as a token, e.g. `HH:mm:ss [hrs]`. A token from the wrong value kind (e.g. `EEEE` against `system.uptime`) fails the whole filter, same as an unsupported value type.

**Breaking change:** before this filter existed, `{{system.time}}` rendered the device's uptime (`H:MM:SS` since boot). It now renders the real wall-clock time of day (`HH:mm:ss`, local timezone). Use `{{system.uptime}}` for the old boot-elapsed behavior - its default text is unchanged.

## Frontend Behavior

The frontend parses and validates placeholders in `portal-spa/src/models/metrics/placeholders.ts`.

Rules:

- Any number of placeholders is allowed in the same text field.
- Valid placeholders are inserted through the placeholder builder UI.
- Manually typed placeholders are validated on every edit.
- A widget text value with no placeholder is treated as static text.
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
- If a placeholder cannot be parsed or cannot be resolved, it is replaced with `N/A` (`kUnresolvedPlaceholderText` in `DisplayTextPlaceholderAst.cpp`) rather than an empty string, so a stuck/unavailable value reads as a visible problem on screen instead of blank space.
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
- `previewNumber` (optional) - the raw number behind `preview`: epoch seconds for `datetime`, milliseconds for `duration`, or the number itself for `int`/`float`. Lets the designer's `format`/`fixed` filter preview reformat a value without re-parsing an already-baked string.

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
- On pixel displays, Text widget width and height still define the render box.
- Placeholder evaluation happens after layout loading and before widget drawing.
- Individual placeholders render as `N/A` if some are unavailable; surrounding static text still renders.
- Filters are intentionally small and bounded, even though `format`/`fixed` now take an argument. New filters should be added only when they map cleanly to typed metric values or existing display formatting rules.

## Related Docs

- [OLED text rendering notes](./oled-text-rendering-notes.md)
- [OLED display layout persistence](./oled-display-layout.md)
- [REST API contract](./rest-api-contract.md)
