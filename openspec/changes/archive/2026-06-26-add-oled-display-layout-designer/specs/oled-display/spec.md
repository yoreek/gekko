## ADDED Requirements

### Requirement: Layout supports typed display widgets
The OLED layout contract SHALL represent each widget with an explicit display widget type and compact drawing attributes.

#### Scenario: API accepts typed widget JSON
- **WHEN** the portal sends a layout widget with `type` equal to `text`, `icon`, `rect`, `line`, or `circle`
- **THEN** the OLED adapter validates the type, geometry, binding fields, bounded text, and supported drawing attributes

#### Scenario: Firmware persists typed widgets
- **WHEN** the firmware stores a typed layout widget
- **THEN** the binary layout payload stores the widget type, geometry, binding fields, bounded text, and compact drawing attributes in versioned widget records

#### Scenario: API returns typed widget JSON
- **WHEN** the portal requests an OLED device whose runtime layout contains typed widgets
- **THEN** the API returns each widget with its `type`, geometry, binding fields, bounded text, and supported drawing attributes under `config.layout.pages[].widgets[]`

#### Scenario: Unsupported widget type is rejected
- **WHEN** the portal sends a layout widget type outside the supported widget set
- **THEN** the OLED adapter rejects the layout as invalid and does not update persisted display layout state

### Requirement: Typed layout remains bounded
The OLED layout contract SHALL keep schema v2 typed widgets within the existing device-scoped sidecar budget.

#### Scenario: Page and widget counts remain bounded
- **WHEN** the firmware parses or encodes a schema v2 layout
- **THEN** it enforces the configured maximum page count and maximum widgets per page

#### Scenario: Geometry is display-bounded
- **WHEN** the firmware parses a typed widget
- **THEN** it rejects widgets whose `x`, `y`, `width`, or `height` are outside the owning OLED display dimensions or cannot be represented by the layout binary record

#### Scenario: String fields remain bounded
- **WHEN** the firmware parses a typed text or icon widget
- **THEN** it rejects values that exceed the bounded OLED layout text capacity

#### Scenario: Maximum layout payload fits
- **WHEN** the firmware encodes the maximum supported schema v2 page/widget layout
- **THEN** the encoded sidecar fits within `kMaxDeviceConfigBytes`

### Requirement: Legacy OLED layout remains readable
The OLED layout codec SHALL preserve compatibility with schema v1 layouts that do not carry explicit widget types.

#### Scenario: Schema v1 binary layout loads
- **WHEN** an OLED runtime loads a persisted schema v1 layout
- **THEN** the codec decodes it and normalizes each legacy widget as a text widget in runtime memory

#### Scenario: Schema v1 JSON layout parses
- **WHEN** the OLED adapter receives a schema v1 JSON layout without widget `type`
- **THEN** it accepts the layout when the existing v1 fields are valid and normalizes widgets as text widgets

#### Scenario: Legacy layout emits current schema JSON
- **WHEN** the API serializes a legacy layout after it has been loaded
- **THEN** it emits the current schema version and includes explicit widget `type` fields

### Requirement: Generic value template bindings are retained
The OLED layout contract SHALL retain source bindings for generic text value templates without defining the full placeholder catalog in this change.

#### Scenario: Template text is stored with binding
- **WHEN** a text widget stores template text containing `{value}`
- **THEN** the layout stores that text together with `bindingKind`, `sourceDeviceId`, and `metricId`

#### Scenario: Missing source does not corrupt layout
- **WHEN** a bound source device is deleted after the layout is saved
- **THEN** the OLED layout remains valid and the runtime may treat that widget value as unavailable

#### Scenario: Unknown placeholder is not expanded by layout codec
- **WHEN** the layout codec parses text containing placeholder-like syntax other than `{value}`
- **THEN** it stores the bounded text literally and does not attempt device-specific placeholder resolution
