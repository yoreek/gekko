## MODIFIED Requirements

### Requirement: Device detail modal
The SPA SHALL open a modal dialog for a selected device and present a shared Device form surface with common fields first and type-specific details below.

#### Scenario: Device details open in modal
- **WHEN** a user selects a device widget or table row
- **THEN** the SPA opens a modal dialog containing the selected device details

#### Scenario: Modal shows shared base fields first
- **WHEN** the detail modal is open
- **THEN** it presents the shared device name, type, and enabled state before type-specific device details

#### Scenario: Modal delegates type-specific details
- **WHEN** the selected device has a registered type-specific detail section
- **THEN** the modal renders that section below the common fields through the device UI registry

#### Scenario: Modal stays synchronized
- **WHEN** the selected device changes due to a realtime update while the modal is open
- **THEN** the modal refreshes its visible state without closing

## ADDED Requirements

### Requirement: Device detail field hints use inline info tooltips
The SPA SHALL show compact `i` icon tooltips for the GPIO switch fields that need explanatory guidance.

#### Scenario: Hints are visible in all form modes
- **WHEN** the user views, edits, or creates a supported GPIO switch device
- **THEN** the fields `Startup state`, `Safe state`, and `Restore previous state` show an inline info icon that opens a tooltip with guidance text

#### Scenario: Hint text is localized
- **WHEN** the tooltip opens in English or Russian
- **THEN** the text comes from the active locale messages rather than hard-coded component copy

#### Scenario: Hints remain available in View mode
- **WHEN** the user opens a supported GPIO switch in View mode
- **THEN** the inline info tooltips remain available alongside the readonly field values

### Requirement: Device form is shared across view edit and create flows
The SPA SHALL use one shared Device form structure for View, Edit, and Create flows while allowing each mode to provide its own submit behavior.

#### Scenario: View mode is readonly by default
- **WHEN** the user opens an existing device in View mode
- **THEN** the shared Device form renders common and type-specific fields as readonly values except for supported runtime commands

#### Scenario: Edit mode reuses the same field order
- **WHEN** the user enters Edit mode for an existing device
- **THEN** the shared Device form keeps common fields before type-specific fields and renders editable controls only for supported fields

#### Scenario: Create mode reveals type fields after type selection
- **WHEN** the user creates a device and selects a device type
- **THEN** the shared Device form renders the selected type-specific create fields below the common fields

#### Scenario: Existing device type is not changed by edit mode
- **WHEN** the user edits an existing device
- **THEN** the device type remains readonly unless a backend contract explicitly supports changing it

### Requirement: Dummy device view remains compact
The SPA SHALL render Dummy device details using only the shared common Device form fields unless a meaningful Dummy-specific field exists.

#### Scenario: Dummy view shows common identity
- **WHEN** the detail modal is open for a Dummy device
- **THEN** the visible form shows name, type, and enabled state without an empty or noisy configuration section

### Requirement: GPIO switch form separates primary and secondary fields
The SPA SHALL present GPIO switch primary operational fields separately from secondary configuration details.

#### Scenario: GPIO switch primary fields stay visible
- **WHEN** the shared Device form renders a GPIO switch in View, Edit, or Create mode
- **THEN** `GPIO pin` and `Output state` are visible in the primary type-specific section when the value is applicable to that mode

#### Scenario: GPIO switch secondary fields are collapsed by default
- **WHEN** the shared Device form renders a GPIO switch
- **THEN** `Startup state`, `Safe mode` or safe state, `Restore previous state`, and `Inverted` are placed under a `Config details` disclosure that is collapsed by default

#### Scenario: GPIO switch create uses defaults until expanded
- **WHEN** the user creates a GPIO switch without expanding or changing `Config details`
- **THEN** the SPA submits the default GPIO switch secondary configuration values

#### Scenario: GPIO switch quick commands stay outside config details
- **WHEN** the shared Device form renders a GPIO switch in View mode
- **THEN** the `On`, `Off`, and `Disabled` quick commands are visible outside the collapsed `Config details` disclosure
