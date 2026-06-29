## Purpose

Define structured metric placeholders for display text widgets, including catalog exposure, runtime formatting, save-time validation, and refresh-driven redraw behavior.

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
The firmware SHALL resolve compiled structured metric placeholders in text widgets at render time and keep the layout renderable when an individual placeholder source is missing.

#### Scenario: Supported placeholder is rendered
- **WHEN** a text widget contains a supported placeholder and the source metric is available
- **THEN** the renderer substitutes the current metric value into the drawn text

#### Scenario: Typed values are formatted before render
- **WHEN** a placeholder resolves to a typed numeric or boolean value
- **THEN** the placeholder pipeline formats that value into text using the filter and display formatting rules

#### Scenario: Multiple placeholders are rendered in order
- **WHEN** a text widget contains more than one supported placeholder mixed with literal text
- **THEN** the renderer substitutes each current metric value while preserving the original literal and placeholder order

#### Scenario: Missing source is soft-failed
- **WHEN** a text widget contains a placeholder whose source device or metric is unavailable at render time
- **THEN** the renderer omits that placeholder value from the rendered text and keeps the rest of the layout renderable

#### Scenario: Stale placeholder does not crash rendering
- **WHEN** a previously compiled text widget contains a placeholder reference that is no longer resolvable at render time
- **THEN** the renderer marks that placeholder value unavailable and continues rendering the display without crashing

#### Scenario: Save validation rejects invalid placeholder syntax
- **WHEN** the portal saves a layout containing malformed structured placeholder syntax
- **THEN** the firmware rejects the layout update instead of accepting the text for later render-time parsing

#### Scenario: Save validation rejects unknown placeholder references
- **WHEN** the portal saves a layout containing a structured placeholder with an unknown source device or metric key
- **THEN** the firmware rejects the layout update instead of accepting an unresolved reference

#### Scenario: Save validation rejects unsupported device metrics
- **WHEN** the portal saves a layout containing a device placeholder whose metric key is not supported by the referenced source device type
- **THEN** the firmware rejects the layout update instead of accepting a placeholder that cannot be resolved for that device type

#### Scenario: Source device dependencies prevent deletion
- **WHEN** a saved display layout references a source device through a structured metric placeholder
- **THEN** the firmware records the source device as a `metric_source` dependency of the display device
- **AND** deleting the source device is rejected while that display layout dependency exists

### Requirement: Dynamic text widgets drive a bounded redraw cadence
The firmware SHALL use compiled text-widget placeholder state and refresh intervals to schedule full display redraws when a layout contains dynamic placeholders.

#### Scenario: Static layout does not schedule periodic redraws
- **WHEN** a display page contains only static widgets
- **THEN** the renderer performs the initial clear-and-draw pass and does not keep a periodic refresh timer

#### Scenario: Dynamic widgets use the minimum interval
- **WHEN** a display page contains more than one dynamic text widget
- **THEN** the renderer schedules the next redraw using the smallest effective refresh interval across those widgets

#### Scenario: Redraw repaints the entire layout
- **WHEN** a scheduled refresh is due
- **THEN** the display is cleared once and all widgets on the active page are drawn in order
