## ADDED Requirements

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
- **THEN** the SPA sends one structured `update_config` command containing the JSON config and deps

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
The SPA SHALL display thermostat dep, control, and output state without one-off styling overrides.

#### Scenario: Detail view shows deps
- **WHEN** the detail modal is open for a thermostat
- **THEN** it shows the selected temperature sensor and switch with their current effective statuses

#### Scenario: Detail view shows latest control state
- **WHEN** a thermostat has runtime output state
- **THEN** the detail view displays mode, target, current temperature when valid, desired switch state, actual switch state when available, and last check context

#### Scenario: Detail view handles unavailable temperature
- **WHEN** a thermostat has no valid or fresh temperature reading, is disabled, or is dependency-blocked
- **THEN** the detail view displays a localized unavailable state without showing stale data or `0` as current temperature

#### Scenario: Widget uses compact device pattern
- **WHEN** a thermostat widget is displayed on the dashboard
- **THEN** it follows the existing compact device widget behavior and may show primary thermostat state only if it fits without changing the fixed widget footprint

#### Scenario: UI follows shared design system
- **WHEN** thermostat UI components are implemented
- **THEN** they use Vuetify components, theme tokens, shared semantic text roles, and avoid local color, font-weight, letter-spacing, opacity, radius, or behavior overrides
