## MODIFIED Requirements

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

### Requirement: Layout text capacity supports multiple placeholders
The OLED layout contract SHALL support text widget values up to 128 bytes so realistic multi-placeholder strings fit in one text widget while keeping display sidecar storage bounded.

#### Scenario: Multi-placeholder text fits in layout storage
- **WHEN** a text widget stores multiple normalized placeholders whose combined text is at most 128 bytes
- **THEN** the layout JSON and binary codecs accept and preserve the raw text value

#### Scenario: Over-capacity text is rejected
- **WHEN** a text widget stores text longer than 128 bytes
- **THEN** the OLED adapter rejects the layout as invalid and does not update persisted display layout state

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

#### Scenario: Legacy template compatibility remains isolated
- **WHEN** a text widget uses `{value}` with `bindingKind` equal to metric
- **THEN** the runtime resolves that legacy template binding without treating `{value}` as a structured `{{...}}` placeholder
