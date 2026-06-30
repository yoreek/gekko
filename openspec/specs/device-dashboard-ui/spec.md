## Purpose

Define the device dashboard widgets, shared device detail form, typed device details, and runtime command behavior in the portal SPA.

## Requirements

### Requirement: Device dashboard cards
The SPA SHALL present the device dashboard as a panel widget surface of compact fixed-size device cards that show only the device display name, with readiness expressed through visual state only.

### Requirement: Device widgets are split by component responsibility
The SPA SHALL organize device widgets in a dedicated component area with a base device widget component and type-specific extensions.

#### Card composition
- The dashboard card MUST show only the device name.
- The dashboard card MAY include an optional compact device-type icon only if it does not increase the card height or add extra metadata rows.
- The dashboard card MUST NOT render `Ready` or `!Ready` text.
- The dashboard card MUST dim the card when the backend status is not `ready`.
- The dashboard card MUST use a fixed compact footprint and MUST NOT expose resize handles.
- The dashboard card MAY be implemented as a reusable base card shell with type-specific specialized components layered on top.

#### Scenario: Base widget is reused
- **WHEN** a device type does not need custom rendering
- **THEN** the SPA renders it with the base device widget component

#### Scenario: Type-specific widget can extend base
- **WHEN** a device type needs custom rendering in the future
- **THEN** the SPA can render it with a type-specific component layered on top of the base widget component

#### Scenario: Device widgets are rendered
- **WHEN** the dashboard loads the active panel layout
- **THEN** it renders one widget per device in the saved panel layout

#### Scenario: Widgets show primary state only
- **WHEN** a device widget is displayed
- **THEN** it shows the device name

#### Scenario: Not ready widgets are dimmed
- **WHEN** a device widget has any backend status other than `ready`
- **THEN** the card is visually dimmed to indicate that it is not working

#### Scenario: Widgets reflect live updates
- **WHEN** a realtime device update arrives
- **THEN** the affected widget updates without requiring a full page reload

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

### Requirement: Device detail dialog shows recent event history
The SPA SHALL expose recent device event history from the shared device detail dialog without filters.

#### Scenario: Recent journal is collapsed by default
- **WHEN** a device detail dialog is opened in view mode
- **THEN** the dialog shows a recent event journal section that is collapsed by default

#### Scenario: Recent journal is limited to selected device
- **WHEN** the user expands the recent event journal section for a device
- **THEN** the section shows only entries whose device ID exactly matches that device

#### Scenario: Recent journal shows latest five events
- **WHEN** more than five retained journal entries exist for the selected device
- **THEN** the section shows only the five newest entries in reverse chronological order

#### Scenario: Recent journal updates dynamically
- **WHEN** a supported realtime message for the selected device arrives while the detail dialog is open
- **THEN** the recent event journal section reflects the new entry without reopening the dialog

### Requirement: Device detail modal supports edit and runtime commands
The SPA SHALL allow the user to rename, enable, disable, and command a device from the detail modal, while destructive registry deletion stays outside the modal.

#### Scenario: Device can be renamed
- **WHEN** the user submits a new name from the modal
- **THEN** the SPA sends the rename through the existing device API path and updates the view on success

#### Scenario: Device can be enabled or disabled
- **WHEN** the user toggles the enabled state from the modal
- **THEN** the SPA sends the corresponding enable or disable action through the existing device API path and reflects the returned state

#### Scenario: Device command can be executed
- **WHEN** the user submits a command from the modal
- **THEN** the SPA sends the command to `POST /api/devices/:id/command` and shows the returned result or validation error

#### Scenario: Delete is not shown in the detail modal
- **WHEN** the detail modal is open in View or Edit mode
- **THEN** the modal does not render a delete button, `Actions` section, or destructive registry action

### Requirement: Bus device detail views surface runtime diagnostics
The SPA SHALL render bus diagnostics in a dedicated runtime section for supported bus device detail views.

#### Scenario: I2C bus detail shows diagnostics
- **WHEN** the user opens an I2C bus device detail view
- **THEN** the UI shows the nested runtime diagnostics counters in a compact diagnostics section

#### Scenario: SPI bus detail shows diagnostics
- **WHEN** the user opens an SPI bus device detail view
- **THEN** the UI shows the nested runtime diagnostics counters in a compact diagnostics section

#### Scenario: Diagnostics stay separate from config
- **WHEN** the bus detail view renders diagnostics
- **THEN** it keeps the diagnostic counters separate from persisted configuration fields

### Requirement: I2C bus detail shows scan state
The SPA SHALL show the current I2C scan state and discovered addresses in the I2C bus detail view when a scan has been requested.

#### Scenario: Scan progress is visible
- **WHEN** an I2C bus scan is in progress
- **THEN** the detail view shows that the scan is active and can display the discovered addresses already collected

#### Scenario: Completed scan is visible
- **WHEN** an I2C bus scan completes
- **THEN** the detail view shows the completed result from runtime state without requiring a full page reload

### Requirement: Bus diagnostics can be reset from the detail view
The SPA SHALL provide a reset action in the supported bus device detail view so operators can clear transient diagnostics without changing configuration.

#### Scenario: Reset action is available
- **WHEN** the user opens a supported bus detail view
- **THEN** the UI exposes a diagnostics reset action for that bus runtime

#### Scenario: Reset action updates the view
- **WHEN** the user triggers diagnostics reset
- **THEN** the UI refreshes the visible diagnostics counters after the backend publishes the updated snapshot

### Requirement: Device commands and config edits use structured request fields
The SPA SHALL submit migrated device commands with named JSON fields and SHALL submit typed device config edits as JSON `config` objects instead of frontend-built binary blobs.

#### Scenario: Rename sends name
- **WHEN** the user renames a device from the SPA
- **THEN** the SPA sends `command = "rename"` with `name` containing the new device name and omits `payload`

#### Scenario: Status command sends status
- **WHEN** the user exposes a set-status action for a device
- **THEN** the SPA sends `command = "setStatus"` with `status` containing the requested status and omits `payload`

#### Scenario: OneWire scan sends scan command
- **WHEN** the user starts a OneWire scan directly or from DS18B20 address selection
- **THEN** the SPA sends `command = "scan"` to the selected OneWire dependency and omits `payload`

#### Scenario: Switch output sends state
- **WHEN** the user controls a switch-like device output
- **THEN** the SPA sends `command = "setOutput"` with a `state` field and omits packed strings such as `state=on`

#### Scenario: DS18B20 edit sends config object
- **WHEN** the user edits a DS18B20 temperature sensor configuration
- **THEN** the SPA sends `command = "updateConfig"` with a JSON `config` object and omits binary `payload`

#### Scenario: OneWire edit sends config object
- **WHEN** the user edits a OneWire bus GPIO pin, internal pull-up, or enabled config state
- **THEN** the SPA sends `command = "updateConfig"` with `config.enabled`, `config.gpioPin`, and `config.internalPullup`

#### Scenario: GPIO switch edit sends config object
- **WHEN** the user edits GPIO switch configuration
- **THEN** the SPA sends `command = "updateConfig"` with named camelCase config fields for enabled state, GPIO pin, startup state, safe state, restore-previous-state, and inversion

#### Scenario: Frontend binary config encoders are removed
- **WHEN** device edit commands are built
- **THEN** the SPA does not call frontend helpers that construct firmware binary config blobs for OneWire or GPIO switch devices

### Requirement: Thermostat create and edit flow
The SPA SHALL let users create and edit thermostat devices by selecting compatible external sensor and switch deps plus thermostat control settings.

#### Scenario: Temperature sensor selection is required
- **WHEN** the user chooses Thermostat in the device create dialog
- **THEN** the form requires selecting an existing temperature-capable device before the create action can submit

#### Scenario: Switch selection is required
- **WHEN** the user chooses Thermostat in the device create dialog
- **THEN** the form requires selecting an existing switch-like device that supports `on` and `off` before the create action can submit

#### Scenario: Missing compatible deps prevent submit
- **WHEN** no compatible temperature sensor or no compatible switch exists
- **THEN** the thermostat create form prevents submission and presents a localized validation state for the missing dep type

#### Scenario: Edit can update config and deps together
- **WHEN** the user edits thermostat settings or dep selections
- **THEN** the SPA sends one structured `updateConfig` command containing the JSON config and deps

### Requirement: Thermostat config fields
The SPA SHALL expose thermostat configuration fields using shared form structure and Vuetify controls.

#### Scenario: Create form includes thermostat settings
- **WHEN** the thermostat create form is shown
- **THEN** it includes mode, target temperature, hysteresis, safe min/max temperatures, check interval, sensor timeout, retry timeout, and minimum switch interval below common device fields

#### Scenario: Mode choices are explicit
- **WHEN** the user edits thermostat mode
- **THEN** the control offers `off`, `heat`, and `cool` choices and submits the selected mode in the config payload

#### Scenario: Temperature bounds are validated locally
- **WHEN** the user edits target, min safe, max safe, or hysteresis values
- **THEN** the form prevents clearly nonnumeric or inconsistent values before submit where local validation can do so

#### Scenario: Timing fields are bounded locally
- **WHEN** the user edits check interval, sensor timeout, retry timeout, or minimum switch interval
- **THEN** the form prevents clearly nonnumeric or below-minimum values before submit where local validation can do so

### Requirement: Thermostat detail and widget display
The SPA SHALL display thermostat dep, control, and output state without one-off styling overrides or low-value last-check clutter.

#### Scenario: Detail view shows deps
- **WHEN** the detail modal is open for a thermostat
- **THEN** it shows the selected temperature sensor and switch with their current effective statuses

#### Scenario: Detail view shows latest control state
- **WHEN** a thermostat has runtime output state
- **THEN** the detail view displays mode, target, current temperature when valid, desired switch state, and actual switch state when available

#### Scenario: Detail view handles unavailable temperature
- **WHEN** a thermostat has no valid or fresh temperature reading, is disabled, or is dependency-blocked
- **THEN** the detail view displays a localized unavailable state without showing stale data or `0` as current temperature

#### Scenario: Widget uses compact device pattern
- **WHEN** a thermostat widget is displayed on the dashboard
- **THEN** it follows the existing compact device widget behavior and may show primary thermostat state only if it fits without changing the fixed widget footprint

#### Scenario: UI follows shared design system
- **WHEN** thermostat UI components are implemented
- **THEN** they use Vuetify components, theme tokens, shared semantic text roles, and avoid local color, font-weight, letter-spacing, opacity, radius, or behavior overrides

### Requirement: Thermostat edit form tracks draft changes
The SPA SHALL enable thermostat Save only when the draft differs from the current persisted device snapshot.

#### Scenario: Changed thermostat values enable Save
- **WHEN** the user changes thermostat target, hysteresis, timing fields, or dep selections in the edit form
- **THEN** the Save action becomes available because the draft no longer matches the persisted device state

#### Scenario: Unchanged thermostat draft keeps Save disabled
- **WHEN** the user opens a thermostat in Edit mode and makes no changes
- **THEN** the Save action remains disabled

### Requirement: Device UI uses dependency terminology
The SPA SHALL model and display device relationships as dependencies and dependents rather than legacy relationship terminology.

#### Scenario: Device model includes deps
- **WHEN** the SPA normalizes a device snapshot
- **THEN** it stores `deps` and computed `hasDeps` and does not require legacy relationship flags

#### Scenario: Labels use dependencies
- **WHEN** the UI displays relationship fields in English or Russian
- **THEN** labels use dependency wording rather than legacy relationship wording

#### Scenario: Mock data uses deps
- **WHEN** the SPA runs in mock mode
- **THEN** mock device records use `config.deps` in the same shape as production snapshots

#### Scenario: Mock mode matches production command shape
- **WHEN** the SPA runs in mock mode
- **THEN** mock command handling accepts the same structured command fields as the production API and rejects migrated legacy payload-only command shapes

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

### Requirement: DS18B20 create flow
The SPA SHALL let users create DS18B20 temperature sensors only after selecting a compatible OneWire dependency and a valid DS18B20 address.

#### Scenario: Dependency selection is required
- **WHEN** the user chooses DS18B20 in the device create dialog
- **THEN** the form requires selecting an existing device whose type is OneWire bus before the create action can submit

#### Scenario: No dependency bus is available
- **WHEN** no OneWire bus devices exist
- **THEN** the DS18B20 create form prevents submission and presents a localized validation state requiring a OneWire bus first

#### Scenario: Manual address can be entered
- **WHEN** the user enters a ROM address manually
- **THEN** the form accepts only a 16-character hex address shape locally and relies on the backend for family code and CRC validation on submit

#### Scenario: Scan selects only DS18B20 candidates
- **WHEN** the user scans the selected OneWire dependency and scan results contain multiple family codes
- **THEN** the address selector lists only family code `28` candidates for DS18B20 selection

### Requirement: DS18B20 config fields
The SPA SHALL expose DS18B20 configuration fields using shared form structure and Vuetify controls.

#### Scenario: Create form includes sensor settings
- **WHEN** the DS18B20 create form is shown
- **THEN** it includes address, resolution, output unit, poll period, report delta, and report-always controls below common device fields

#### Scenario: Resolution choices are bounded
- **WHEN** the user edits resolution
- **THEN** the control offers only 9, 10, 11, and 12 bit choices

#### Scenario: Unit choices are explicit
- **WHEN** the user edits output unit
- **THEN** the control offers Celsius and Fahrenheit choices and submits the selected unit in the config payload

#### Scenario: Poll period is bounded
- **WHEN** the user edits update period
- **THEN** the form prevents nonnumeric or below-minimum values before submit where local validation can do so

#### Scenario: Report delta is explicit
- **WHEN** the user edits report-on-change behavior
- **THEN** the form exposes a minimum change threshold in Celsius with a default of `0.01 C`

### Requirement: DS18B20 detail and widget display
The SPA SHALL display DS18B20 temperature output and configuration without one-off styling overrides.

#### Scenario: Detail view shows latest temperature
- **WHEN** a DS18B20 device has a valid temperature output
- **THEN** the detail view displays the value with the configured unit and measured timestamp context

#### Scenario: Detail view handles missing reading
- **WHEN** a DS18B20 device has no valid reading, is disabled, or is dependency-blocked
- **THEN** the detail view uses `output.temperature.valid = false` to display a localized unavailable state without showing stale data or `0` as current

#### Scenario: Widget uses compact device pattern
- **WHEN** a DS18B20 widget is displayed on the dashboard
- **THEN** it follows the existing compact device widget behavior and may show the primary temperature value only if it fits without changing the fixed widget footprint

#### Scenario: UI follows shared design system
- **WHEN** DS18B20 UI components are implemented
- **THEN** they use Vuetify components, theme tokens, shared semantic text roles, and avoid local color, font-weight, letter-spacing, opacity, radius, or behavior overrides
