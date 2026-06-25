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
- **THEN** the OLED adapter validates that JSON and converts it into an opaque binary sidecar payload

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
The OLED runtime SHALL keep the active layout as a typed runtime struct.

#### Scenario: Runtime layout is typed
- **WHEN** the OLED runtime has an active layout
- **THEN** it stores it as `OledDisplayLayoutRecordV1`

#### Scenario: Runtime layout may use dynamic vectors
- **WHEN** the runtime keeps pages and widgets in RAM
- **THEN** it may use dynamic vectors for pages and widgets while still persisting the layout as binary

#### Scenario: Runtime normalizes created layout device ID
- **WHEN** an OLED layout sidecar created before device ID assignment is applied to a runtime
- **THEN** the runtime rewrites `layout.deviceId` to its own `deviceId()`

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
