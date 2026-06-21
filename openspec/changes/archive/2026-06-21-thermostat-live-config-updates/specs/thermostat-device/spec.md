## ADDED Requirements

### Requirement: Thermostat stable config application
The thermostat SHALL apply validated config updates to the existing runtime object and SHALL reset its state machine only when dependency links actually change.

#### Scenario: Control settings update without state-machine reset
- **WHEN** an accepted thermostat update changes only mode, algorithm, target temperature, safe range, hysteresis, check interval, sensor timeout, retry timeout, or minimum switch interval
- **THEN** the thermostat applies the new config to the existing runtime object, keeps dependency runtime bindings, and does not reset its state machine solely because those fields changed

#### Scenario: Control settings use next cooperative evaluation
- **WHEN** a thermostat config-only update changes mode, target, safe range, hysteresis, or timing values
- **THEN** the next thermostat control evaluation uses the new config without calling `millis()` or blocking the main loop

#### Scenario: Dependency reassignment resets thermostat
- **WHEN** an accepted thermostat update changes the `temperature_sensor` or `switch` dependency device id
- **THEN** the registry relinks the thermostat dependency runtime pointers and resets the thermostat state machine to its initial `Idle` state

#### Scenario: Switch dependency change does not command old switch
- **WHEN** an accepted thermostat update changes the `switch` dependency device id
- **THEN** the thermostat does not send an output command to the previously linked switch as part of the dependency change

#### Scenario: Invalid thermostat update is rejected atomically
- **WHEN** a thermostat config update contains an invalid mode, unsupported algorithm, invalid safe range, invalid hysteresis, invalid timing value, or invalid dependency relationship
- **THEN** the firmware rejects the update before changing the thermostat runtime config, dependency links, config revision, or persistence state
