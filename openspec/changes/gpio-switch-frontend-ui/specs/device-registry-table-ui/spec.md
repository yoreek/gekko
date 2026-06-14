## ADDED Requirements

### Requirement: GPIO switch create and edit forms
The SPA SHALL provide type-specific form fields for creating and editing `GpioSwitchDevice` records.

#### Scenario: Shared switch fields are reused
- **WHEN** a `GpioSwitchDevice` create or edit form is rendered
- **THEN** it uses shared switch form components for startup state, safe state, restore-previous-state, and inversion rather than duplicating those fields in the GPIO-specific component

#### Scenario: GPIO switch create fields are shown
- **WHEN** the user selects `GpioSwitchDevice` in the create-device flow
- **THEN** the form shows fields for GPIO pin, startup state, safe state, restore-previous-state, and inversion

#### Scenario: GPIO switch form uses Vuetify controls
- **WHEN** the GPIO switch form is rendered
- **THEN** it uses ready-made Vuetify input, select, switch, button, and validation components instead of custom form primitives

#### Scenario: GPIO switch create payload is submitted
- **WHEN** the user submits a valid GPIO switch create form
- **THEN** the SPA sends `type_id = 2` and the GPIO switch config JSON expected by the existing device API

#### Scenario: GPIO switch edit payload is submitted
- **WHEN** the user updates GPIO switch configuration from the detail surface
- **THEN** the SPA sends the updated config through the existing device update API without changing unrelated shared fields

### Requirement: GPIO switch details and controls
The SPA SHALL render a typed details section for `GpioSwitchDevice` with explicit output-state controls.

#### Scenario: Shared switch controls are reused
- **WHEN** a `GpioSwitchDevice` detail section renders output controls
- **THEN** it uses the shared switch output-control component that can also support future switch device types

#### Scenario: GPIO switch details are shown
- **WHEN** the selected device is a `GpioSwitchDevice`
- **THEN** the detail dialog shows shared device fields plus GPIO pin, startup state, safe state, restore-previous-state, inversion, and runtime `output.state` when available

#### Scenario: GPIO switch can be set on
- **WHEN** the user chooses the `on` output command
- **THEN** the SPA sends the command to `POST /api/devices/:id/command` and refreshes the displayed output state from the returned, pushed, or reloaded device data

#### Scenario: GPIO switch can be set off
- **WHEN** the user chooses the `off` output command
- **THEN** the SPA sends the command to `POST /api/devices/:id/command` and refreshes the displayed output state from the returned, pushed, or reloaded device data

#### Scenario: GPIO switch can be disabled
- **WHEN** the user chooses the `disabled` output command for a GPIO switch
- **THEN** the SPA sends the command to `POST /api/devices/:id/command` and refreshes the displayed output state from the returned, pushed, or reloaded device data

#### Scenario: Unsupported command state is not sent
- **WHEN** the UI cannot resolve a selected output state to `on`, `off`, or `disabled`
- **THEN** it prevents submission and shows a validation error instead of sending an invalid command

### Requirement: GPIO switch mock behavior
The SPA mock API SHALL support GPIO switch config and command behavior for local browser checks.

#### Scenario: Mock creates GPIO switch
- **WHEN** mock mode receives a valid `GpioSwitchDevice` create request
- **THEN** it stores the GPIO switch config fields and returns a device record that appears in dashboard and Devices views

#### Scenario: Mock command updates output state
- **WHEN** mock mode receives a GPIO switch command for `on`, `off`, or `disabled`
- **THEN** it updates the mock device output state and returns a successful command response

#### Scenario: Mock rejects unsupported output state
- **WHEN** mock mode receives an unsupported GPIO switch command state
- **THEN** it returns a validation error and leaves the mock device state unchanged
