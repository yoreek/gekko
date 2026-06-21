## ADDED Requirements

### Requirement: GPIO switch config update impact
The GPIO switch runtime SHALL classify config fields by whether they require hardware cleanup and state-machine reset while preserving the existing runtime object.

#### Scenario: GPIO pin change releases old pin and restarts
- **WHEN** an accepted GPIO switch config update changes `gpio_pin`
- **THEN** the firmware calls the switch runtime `end` hook while the old pin config is still active, applies the new config to the existing runtime object, and resets the state machine to its initial `Idle` state

#### Scenario: Inversion change restarts switch runtime
- **WHEN** an accepted GPIO switch config update changes `inverted`
- **THEN** the firmware calls the switch runtime `end` hook, applies the new config to the existing runtime object, and resets the state machine to its initial `Idle` state

#### Scenario: Switch reset uses startup or retained logical output
- **WHEN** a GPIO switch state machine restarts after a `gpio_pin` or `inverted` config update
- **THEN** the switch chooses its logical output using the normal startup-state or retained-state startup behavior rather than preserving the pre-update logical output

#### Scenario: Switch policy fields do not restart
- **WHEN** an accepted GPIO switch config update changes only `startup_state`, `safe_state`, or `restore_previous_state`
- **THEN** the firmware applies the new config to the existing runtime object without calling `end` or resetting the switch state machine

#### Scenario: Invalid switch config is rejected before cleanup
- **WHEN** a GPIO switch config update references an unsupported pin or invalid switch state value
- **THEN** the firmware rejects the update before calling `end`, changing runtime config, resetting the state machine, or changing persistence state
