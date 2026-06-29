## Purpose

Define how an OLED display device owns, exposes, and persists its layout through the existing device registry and device-scoped storage model.

## Requirements

### Requirement: OLED display is a registry device on I2C
The firmware SHALL model the OLED display as a device that depends on an I2C bus device.

#### Scenario: Display binds to an I2C bus
- **WHEN** a user creates or edits an OLED display device
- **THEN** the device stores the selected `i2cBusDeviceId` dependency and I2C display address in the main device config

### Requirement: Layout is device-owned data
The firmware SHALL treat OLED layout as device-owned data separate from the main registry config blob.

#### Scenario: Layout is stored separately
- **WHEN** the user saves pages or widgets for an OLED display
- **THEN** the firmware persists layout under the owning device ID using the `display_layout` device-scoped key

#### Scenario: Layout is cleared with the device
- **WHEN** the OLED display device is deleted
- **THEN** the firmware clears the stored `display_layout` payload for that device

### Requirement: Layout uses JSON only at the API boundary
The portal SHALL exchange OLED layout as JSON, while firmware persistence SHALL remain binary.

#### Scenario: API accepts JSON layout
- **WHEN** the portal sends `config.layout` in an OLED create or update request
- **THEN** the OLED adapter validates that JSON, validates structured text placeholders, extracts metric source dependencies, and converts it into an opaque binary sidecar payload

#### Scenario: Placeholder filters survive layout validation
- **WHEN** the portal sends `config.layout` containing `{{dev.123.temperature | upper}}`
- **THEN** the OLED adapter validates the placeholder body and the trailing filter as one structured text expression
- **AND** it keeps the raw widget `text` unchanged for persistence

#### Scenario: API rejects invalid text placeholders
- **WHEN** the portal sends `config.layout` containing malformed placeholder syntax, an unknown placeholder namespace, an unknown source device, or an unknown metric key
- **THEN** the OLED adapter rejects the request with a validation error
- **AND** it does not update persisted display layout state

#### Scenario: API persists metric source dependencies
- **WHEN** the portal saves a layout containing `dev` placeholders that reference source devices
- **THEN** the firmware stores those source devices as `metric_source` dependency links on the display device
- **AND** it preserves the display hardware bus dependency link

#### Scenario: API returns JSON layout
- **WHEN** the portal requests OLED device data
- **THEN** the runtime serializes its in-memory layout struct back into JSON under `config.layout`

### Requirement: Layout update flows through generic persisted-state hooks
The firmware SHALL apply OLED layout changes through the generic persisted-state lifecycle rather than OLED-specific boot or controller code.

#### Scenario: Create applies layout after device ID assignment
- **WHEN** the user creates an OLED display with `config.layout`
- **THEN** the controller first creates the device through the normal registry flow
- **AND THEN** it applies the opaque persisted-state sidecar through the generic registry persisted-state API

#### Scenario: Update applies layout after config update
- **WHEN** the user sends the standard OLED `updateConfig` command with `config.layout`
- **THEN** the controller first updates the main config blob
- **AND THEN** it applies the opaque persisted-state sidecar through the generic registry persisted-state API

#### Scenario: Boot reload restores layout through generic load hook
- **WHEN** the registry recreates OLED runtimes during `begin(...)`
- **THEN** it restores layout by calling the generic persisted-state load hook on the runtime

### Requirement: Layout is stored as versioned binary records
The firmware SHALL persist OLED layout as a versioned binary payload with explicit header, page, and widget records.

#### Scenario: Binary header is explicit
- **WHEN** the firmware serializes OLED layout
- **THEN** the payload starts with a header containing `recordVersion`, `deviceId`, `schemaVersion`, `activePageIndex`, and `pageCount`

#### Scenario: Page records are explicit
- **WHEN** the firmware serializes a page
- **THEN** it stores a bounded page ID and widget count before that page's widget records

#### Scenario: Widget records are explicit
- **WHEN** the firmware serializes a widget
- **THEN** it stores binding kind, geometry, source device ID, metric ID, and bounded text fields

#### Scenario: Unsupported binary version is rejected
- **WHEN** the firmware loads a persisted OLED layout with an unsupported binary or schema version
- **THEN** it rejects the payload as invalid and does not restore it into the runtime

### Requirement: Runtime owns layout as a struct
The OLED runtime SHALL keep the active layout as a typed runtime struct plus transient compiled text data derived from that struct.

#### Scenario: Runtime layout is typed
- **WHEN** the OLED runtime has an active layout
- **THEN** it stores it as `OledDisplayLayoutRecordV1`

#### Scenario: Runtime layout may use dynamic vectors
- **WHEN** the runtime keeps pages and widgets in RAM
- **THEN** it may use dynamic vectors for pages and widgets while still persisting the layout as binary

#### Scenario: Runtime normalizes created layout device ID
- **WHEN** an OLED layout sidecar created before device ID assignment is applied to a runtime
- **THEN** the runtime rewrites `layout.deviceId` to its own `deviceId()`

#### Scenario: Runtime invalidates text widgets after layout load
- **WHEN** the OLED runtime loads or replaces a layout
- **THEN** it invalidates transient compiled text data derived from previous raw text widget values

#### Scenario: Runtime builds text AST lazily
- **WHEN** the OLED runtime renders a text widget whose compiled AST is missing or invalid
- **THEN** it builds the transient AST from the raw text widget value before evaluating placeholders

#### Scenario: Runtime formats typed metric values
- **WHEN** the OLED runtime resolves a metric placeholder whose source returns a typed numeric, boolean, or string value
- **THEN** the runtime formats that typed value for display after placeholder resolution and before layout draw

### Requirement: Layout bounds are enforced
The firmware SHALL keep OLED layout bounded and reject invalid payloads.

#### Scenario: Page count is bounded
- **WHEN** the firmware loads or parses OLED layout
- **THEN** it rejects layouts whose page count exceeds the supported maximum

#### Scenario: Widget count is bounded
- **WHEN** the firmware loads or parses OLED layout
- **THEN** it rejects layouts whose widget count on any page exceeds the supported maximum

#### Scenario: String fields are bounded
- **WHEN** the firmware loads or parses OLED layout
- **THEN** it rejects layouts whose page IDs or widget text exceed the supported capacities

#### Scenario: Active page index is valid
- **WHEN** the firmware loads or parses OLED layout
- **THEN** it rejects layouts whose `activePageIndex` is outside the current page count

### Requirement: Widget bindings use stable device IDs
The firmware SHALL bind OLED widgets to source devices by stable device ID.

#### Scenario: Binding survives rename
- **WHEN** a bound source device is renamed
- **THEN** the OLED widget binding remains valid because it is keyed by device ID

#### Scenario: Missing source is handled safely
- **WHEN** a bound source device is removed
- **THEN** the OLED runtime may treat the widget as unavailable rather than dereferencing stale data

### Requirement: Layout supports typed display widgets
The OLED layout contract SHALL represent each widget with an explicit display widget type and compact drawing attributes.

#### Scenario: API accepts typed widget JSON
- **WHEN** the portal sends a layout widget with `type` equal to `text`, `icon`, `rect`, `line`, `circle`, or `ellipse`
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
The OLED layout contract SHALL retain source bindings for generic text value templates while routing structured `{{...}}` placeholders through the compiled placeholder parser.

#### Scenario: Template text is stored with binding
- **WHEN** a text widget stores template text containing `{value}`
- **THEN** the layout stores that text together with `bindingKind`, `sourceDeviceId`, and `metricId`

#### Scenario: Missing source does not corrupt layout
- **WHEN** a bound source device is deleted after the layout is saved
- **THEN** the OLED layout remains valid and the runtime may treat that widget value as unavailable

#### Scenario: Structured placeholders are validated by the parser
- **WHEN** the layout codec parses text containing `{{...}}` structured placeholder syntax
- **THEN** it validates the placeholder through the shared placeholder parser before accepting API layout updates

#### Scenario: Unknown placeholder is not expanded by layout codec
- **WHEN** the layout codec parses text containing placeholder-like syntax other than `{value}`
- **THEN** it stores the bounded text literally and does not attempt device-specific placeholder resolution

#### Scenario: Legacy template compatibility remains isolated
- **WHEN** a text widget uses `{value}` with `bindingKind` equal to metric
- **THEN** the runtime resolves that legacy template binding without treating `{value}` as a structured `{{...}}` placeholder
