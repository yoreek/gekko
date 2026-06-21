## Purpose

Define the firmware contract for the composite thermostat device type.

## Requirements

### Requirement: Thermostat device type
The firmware SHALL provide a `ThermostatDevice` dynamic device type with stable type identity, bounded versioned config, and no embedded sensor or switch hardware ownership.

#### Scenario: Thermostat type is registered
- **WHEN** the default firmware device type registry is created after dependency support exists
- **THEN** it includes `ThermostatDevice` with stable `type_id = 5`, current config version, required `temperature_sensor` and `switch` deps, no retained state, and a declared runtime cadence

#### Scenario: Thermostat config is bounded
- **WHEN** a thermostat config is encoded for registry storage
- **THEN** the payload fits within `kMaxDeviceConfigBytes` and contains only thermostat settings, not selected sensor or switch device ids

#### Scenario: Deps are external
- **WHEN** a thermostat device is created
- **THEN** the selected temperature sensor and switch are stored as registry `deps` links rather than nested thermostat config fields

### Requirement: Thermostat dependency roles
The thermostat SHALL require exactly one compatible temperature sensor dep and exactly one compatible switch-like dep.

#### Scenario: Compatible deps are accepted
- **WHEN** a create or update request references one temperature-capable device as `temperature_sensor` and one switch-like device that supports `on` and `off` as `switch`
- **THEN** the firmware accepts the dep set if the graph remains valid

#### Scenario: Missing sensor is rejected
- **WHEN** a thermostat create or update request omits the `temperature_sensor` dep
- **THEN** the firmware rejects the mutation and leaves the existing registry state unchanged

#### Scenario: Missing switch is rejected
- **WHEN** a thermostat create or update request omits the `switch` dep
- **THEN** the firmware rejects the mutation and leaves the existing registry state unchanged

#### Scenario: Incompatible dep is rejected
- **WHEN** a thermostat dep role references a device that does not expose the required runtime capability for that role
- **THEN** the firmware rejects the mutation as an invalid relationship

#### Scenario: Runtime caches typed deps
- **WHEN** the registry wires thermostat deps
- **THEN** the thermostat runtime can access the selected temperature sensor and switch through role-specific runtime fields or equivalent cached pointers

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

### Requirement: Hysteresis thermostat control
The thermostat SHALL implement cooperative hysteresis control for `off`, `heat`, and `cool` modes.

#### Scenario: Heat mode turns switch on below lower threshold
- **WHEN** the thermostat is in heat mode and the latest valid temperature is at or below `target - hysteresis / 2`
- **THEN** the thermostat requests switch output `on`

#### Scenario: Heat mode turns switch off above upper threshold
- **WHEN** the thermostat is in heat mode and the latest valid temperature is at or above `target + hysteresis / 2`
- **THEN** the thermostat requests switch output `off`

#### Scenario: Cool mode turns switch on above upper threshold
- **WHEN** the thermostat is in cool mode and the latest valid temperature is at or above `target + hysteresis / 2`
- **THEN** the thermostat requests switch output `on`

#### Scenario: Cool mode turns switch off below lower threshold
- **WHEN** the thermostat is in cool mode and the latest valid temperature is at or below `target - hysteresis / 2`
- **THEN** the thermostat requests switch output `off`

#### Scenario: Temperature inside hysteresis band holds demand
- **WHEN** the latest valid temperature remains between the lower and upper hysteresis thresholds
- **THEN** the thermostat preserves the previous desired switch output instead of toggling

#### Scenario: Off mode forces switch off
- **WHEN** the thermostat mode is `off`
- **THEN** the thermostat requests switch output `off` and does not run heat or cool threshold control

### Requirement: Thermostat runtime remains cooperative
The thermostat SHALL model control, waiting, retry, dependency blocking, disable, fault, and delete behavior as explicit `StateMachine` states using the provided tick timestamp.

#### Scenario: Check interval uses tick time
- **WHEN** the thermostat waits for the next temperature check
- **THEN** it evaluates `check_interval_ms` using the `now` value passed into its registry cadence tick and does not call `millis()` or `clock_.millis()`

#### Scenario: Retry after sensor error is non-blocking
- **WHEN** the thermostat enters retry after a sensor timeout or out-of-range reading
- **THEN** it waits for `retry_after_error_ms` through state-machine deadlines without blocking the main loop

#### Scenario: Disable path is explicit
- **WHEN** the thermostat is disabled
- **THEN** it enters an explicit disabled state, requests switch output `off`, and stops normal control checks until enabled again

#### Scenario: Delete path is explicit
- **WHEN** the thermostat is deleted
- **THEN** it enters an explicit deleting state and requests switch output `off` before the runtime is removed

### Requirement: Thermostat safety handling
The thermostat SHALL fail safe when sensor data is invalid, stale, out of configured safe range, or deps are unavailable.

#### Scenario: Stale sensor reading faults thermostat
- **WHEN** the temperature sensor dep is ready but its latest valid reading is older than `sensor_timeout_ms`
- **THEN** the thermostat requests switch output `off` and reports a fault or sensor-timeout runtime status until retry succeeds

#### Scenario: Out-of-range reading faults thermostat
- **WHEN** the latest valid temperature is below `min_safe` or above `max_safe`
- **THEN** the thermostat requests switch output `off` and reports an out-of-range fault state

#### Scenario: Dep blocked forces safe output
- **WHEN** either required dep is missing or not effectively ready
- **THEN** the thermostat requests switch output `off` when a switch runtime is available and reports dependency-blocked effective status

#### Scenario: Disabled dep disables thermostat effectively
- **WHEN** either required dep is effectively disabled
- **THEN** the thermostat effective status is `disabled` and normal control work does not run

### Requirement: Thermostat switch output control
The thermostat SHALL command switch output through the switch-like runtime capability while respecting a bounded minimum switch interval for ordinary toggles.

#### Scenario: Switch request updates output
- **WHEN** thermostat control computes a new desired `on` or `off` state and the minimum switch interval allows the change
- **THEN** the switch runtime applies that output state and its runtime output is visible in snapshots

#### Scenario: Minimum switch interval prevents chatter
- **WHEN** thermostat control wants to toggle the switch before `min_switch_interval_ms` has elapsed since the last ordinary output change
- **THEN** the thermostat keeps the previous switch output until the interval expires

#### Scenario: Safety off bypasses chatter guard
- **WHEN** the thermostat is disabled, faulted, deleting, or dependency-blocked
- **THEN** the thermostat may request safe `off` output immediately even if `min_switch_interval_ms` has not elapsed

#### Scenario: Switch retained state remains consistent
- **WHEN** a thermostat-driven switch output change succeeds and the switch supports retained output state
- **THEN** the registry captures the switch retained state through the same persistence path used for direct switch commands

### Requirement: Thermostat snapshot output
The thermostat SHALL expose canonical config, deps, control, and output state through device snapshots.

#### Scenario: Config codec accepts canonical and alias temperature fields
- **WHEN** a thermostat config update includes canonical fixed-point fields or Celsius aliases for target temperature, safe range, or hysteresis
- **THEN** the firmware parses the request, preserves canonical fixed-point values, and stores the requested thermostat settings without falling back to defaults

#### Scenario: Snapshot includes thermostat config
- **WHEN** the API serializes a thermostat device
- **THEN** the snapshot includes mode, target temperature, hysteresis, safe min/max, check interval, sensor timeout, retry timeout, and minimum switch interval

#### Scenario: Snapshot includes dep roles
- **WHEN** the API serializes a thermostat device
- **THEN** the snapshot includes `temperature_sensor` and `switch` dep links with role names, device ids, and current dep statuses

#### Scenario: Snapshot includes control output
- **WHEN** the thermostat has evaluated control state
- **THEN** the snapshot includes latest temperature validity, latest temperature value when available, desired switch output, actual switch output when available, and last check timestamp

#### Scenario: Snapshot remains valid before first reading
- **WHEN** the thermostat has not yet received a valid sensor reading
- **THEN** the snapshot remains valid JSON and marks temperature output unavailable instead of reporting `0` as a current reading
