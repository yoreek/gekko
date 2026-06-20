## MODIFIED Requirements

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

## ADDED Requirements

### Requirement: Thermostat edit form tracks draft changes
The SPA SHALL enable thermostat Save only when the draft differs from the current persisted device snapshot.

#### Scenario: Changed thermostat values enable Save
- **WHEN** the user changes thermostat target, hysteresis, timing fields, or dep selections in the edit form
- **THEN** the Save action becomes available because the draft no longer matches the persisted device state

#### Scenario: Unchanged thermostat draft keeps Save disabled
- **WHEN** the user opens a thermostat in Edit mode and makes no changes
- **THEN** the Save action remains disabled
