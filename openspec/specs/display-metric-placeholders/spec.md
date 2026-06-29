## Purpose

Define structured metric placeholders for display text widgets, including catalog exposure, soft-fail resolution, and bounded refresh-driven redraw behavior.

## Requirements

### Requirement: Metric placeholder catalog is exposed by the firmware
The firmware SHALL expose a structured placeholder catalog for display designers with explicit namespaces for device, system, and wifi values.

#### Scenario: Device metrics include source labels
- **WHEN** the portal requests the metric placeholder catalog
- **THEN** the firmware returns device placeholders with stable device IDs, metric keys, and a human-readable source label

#### Scenario: Global metrics are namespaced
- **WHEN** the portal requests the metric placeholder catalog
- **THEN** the firmware returns system and wifi placeholders without a device source ID and with the correct namespace

### Requirement: Text widgets resolve structured metric placeholders
The firmware SHALL resolve structured metric placeholders in text widgets at render time and keep the layout valid when a source is missing.

#### Scenario: Supported placeholder is rendered
- **WHEN** a text widget contains a supported placeholder and the source metric is available
- **THEN** the renderer substitutes the current metric value into the drawn text

#### Scenario: Missing source is soft-failed
- **WHEN** a text widget contains a placeholder whose source device or metric is unavailable
- **THEN** the renderer omits that placeholder value from the rendered text and keeps the rest of the layout renderable

#### Scenario: Invalid placeholder does not crash rendering
- **WHEN** a text widget contains malformed placeholder syntax
- **THEN** the renderer marks the value unavailable and continues rendering the display without crashing

### Requirement: Dynamic text widgets drive a bounded redraw cadence
The firmware SHALL use text-widget refresh intervals to schedule full display redraws when a layout contains dynamic placeholders.

#### Scenario: Static layout does not schedule periodic redraws
- **WHEN** a display page contains only static widgets
- **THEN** the renderer performs the initial clear-and-draw pass and does not keep a periodic refresh timer

#### Scenario: Dynamic widgets use the minimum interval
- **WHEN** a display page contains more than one dynamic text widget
- **THEN** the renderer schedules the next redraw using the smallest effective refresh interval across those widgets

#### Scenario: Redraw repaints the entire layout
- **WHEN** a scheduled refresh is due
- **THEN** the display is cleared once and all widgets on the active page are redrawn in order
