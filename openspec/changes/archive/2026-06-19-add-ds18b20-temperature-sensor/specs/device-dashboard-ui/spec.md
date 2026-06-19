## ADDED Requirements

### Requirement: DS18B20 create flow
The SPA SHALL let users create DS18B20 temperature sensors only after selecting a compatible OneWire parent and a valid DS18B20 address.

#### Scenario: Parent selection is required
- **WHEN** the user chooses DS18B20 in the device create dialog
- **THEN** the form requires selecting an existing device whose type is OneWire bus before the create action can submit

#### Scenario: No parent bus is available
- **WHEN** no OneWire bus devices exist
- **THEN** the DS18B20 create form prevents submission and presents a localized validation state requiring a OneWire bus first

#### Scenario: Manual address can be entered
- **WHEN** the user enters a ROM address manually
- **THEN** the form accepts only a 16-character hex address shape locally and relies on the backend for family code and CRC validation on submit

#### Scenario: Scan selects only DS18B20 candidates
- **WHEN** the user scans the selected OneWire parent and scan results contain multiple family codes
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
