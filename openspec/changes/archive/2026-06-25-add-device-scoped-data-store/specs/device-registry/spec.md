## MODIFIED Requirements

### Requirement: Operation-level persistence policy
The firmware SHALL support immediate, delayed, and coalesced persistence policies for registry, configuration, and device-scoped data mutations.

#### Scenario: Immediate mutation succeeds
- **WHEN** a mutation with immediate persistence policy is accepted
- **THEN** the firmware writes the required NVS index, record, or device-scoped data changes before returning success and reports no pending persistence for that mutation

#### Scenario: Immediate mutation fails to persist
- **WHEN** a mutation with immediate persistence policy cannot write the required NVS changes
- **THEN** the firmware rejects or rolls back the in-memory mutation and reports the operation as failed

#### Scenario: Delayed mutation is accepted
- **WHEN** a mutation with delayed persistence policy is accepted
- **THEN** the firmware updates the in-memory registry, marks the affected index, device record, or device-scoped data dirty, and reports pending persistence for the affected scope

#### Scenario: Coalesced retained-state mutation is accepted
- **WHEN** a retained-state value changes repeatedly before the flush policy is due
- **THEN** the firmware keeps only the latest retained-state value dirty for that device and avoids writing every intermediate value to NVS

#### Scenario: Dirty records are flushed
- **WHEN** the dirty flush debounce or max-delay policy is reached on an App-scheduled registry tick
- **THEN** the firmware writes only dirty config records and device-scoped data records to NVS, writes the bounded index record and registry format commit marker last when the index is dirty, and clears the selected dirty batch as one unit after the flush attempt

#### Scenario: Flush failure is reported without dirty recovery bookkeeping
- **WHEN** a dirty registry flush cannot write one of the required records, device-scoped data records, index record, or registry format commit marker
- **THEN** the firmware returns the persistence error and does not add per-record dirty recovery bookkeeping

#### Scenario: Forced flush is requested
- **WHEN** the firmware is about to perform a controlled reboot, OTA restart, runtime-control restart API action, factory reset, or explicit save/apply action
- **THEN** the firmware attempts to flush pending dirty registry, config, and device-scoped data records before continuing the controlled action

#### Scenario: Runtime-control restart flush fails
- **WHEN** a runtime-control restart API request cannot flush pending registry persistence
- **THEN** the firmware rejects the restart action and keeps running without initiating reboot

### Requirement: Registry captures runtime-driven retained state
The firmware SHALL capture retained runtime state changes caused by internal runtime interactions and persist them through the device-scoped data mechanism, not only through public REST commands.

#### Scenario: Thermostat changes switch output
- **WHEN** a thermostat runtime changes a switch-like dep output and the switch marks retained state dirty
- **THEN** the registry records the switch retained state through the coalesced device-scoped data persistence path

#### Scenario: Failed internal output does not persist retained state
- **WHEN** a runtime-driven switch output request fails
- **THEN** the registry does not persist a retained output state for that failed request

#### Scenario: Retained capture remains bounded
- **WHEN** runtime-driven output changes occur repeatedly before the flush policy is due
- **THEN** the registry coalesces retained-state persistence using the existing dirty tracking and device-scoped data bounds
- **AND** the firmware rejects the restart action and keeps running without initiating reboot

### Requirement: Retained runtime state storage
The firmware SHALL persist selected frequently changing restore values as device-scoped data separately from device configuration records.

#### Scenario: Switch-like last state changes
- **WHEN** a switch-like device changes its output state and the device type supports restoring the previous state
- **THEN** the firmware records the retained state in device-scoped data instead of modifying the device configuration payload

#### Scenario: Device restores retained state
- **WHEN** a device starts and its configuration requests restore-from-previous behavior
- **THEN** the firmware loads the device's retained state by device ID from device-scoped data and applies it as startup state when the retained state is valid for that device type

#### Scenario: Retained state is missing
- **WHEN** a device requests restore-from-previous behavior but no valid retained state exists
- **THEN** the firmware uses the device type's configured fallback startup state

#### Scenario: Retained state writes are bounded
- **WHEN** retained state changes repeatedly in a short interval
- **THEN** the firmware coalesces, debounces, or otherwise bounds retained-state writes to avoid excessive NVS wear

### Requirement: Registry supports switch-like retained output state
The firmware SHALL support retained output state for switch-like runtimes without requiring switch output changes to rewrite the device configuration record, and it SHALL persist that output state through device-scoped retained-state data.

#### Scenario: Switch runtime output changes
- **WHEN** a switch-like runtime changes its output state and restore-previous-state is enabled
- **THEN** the registry persistence flow can persist the latest state through device-scoped retained-state data using the device id

#### Scenario: Switch runtime restore disabled
- **WHEN** a switch-like runtime changes its output state and restore-previous-state is disabled
- **THEN** the registry persistence flow does not persist switch retained output state

#### Scenario: Switch runtime starts with retained state
- **WHEN** the registry creates a switch-like runtime that supports restore-previous-state
- **THEN** it loads retained state by device id from device-scoped data and applies it before the runtime reaches Ready when the retained payload is valid

### Requirement: Device lifecycle status
The firmware SHALL expose lifecycle status for each dynamic device from creation through configuration, runtime operation, disabling, fault handling, reconfiguration, and deletion.

#### Scenario: Device starts after creation
- **WHEN** a valid enabled device is created
- **THEN** the firmware moves the device through an explicit creation/start lifecycle and exposes its resulting runtime status

#### Scenario: Device configuration changes
- **WHEN** a caller updates a valid configuration for an existing enabled device
- **THEN** the firmware records the accepted configuration revision, applies the operation's persistence policy, and applies the new config to the existing runtime object without recreating that runtime object

#### Scenario: Config update without reset preserves state-machine position
- **WHEN** a valid config update changes only fields that the device type classifies as not requiring reconfiguration
- **THEN** the firmware applies the config to the existing runtime and does not reset the runtime state machine solely because the config changed

#### Scenario: Config update requiring reset uses existing runtime
- **WHEN** a valid config update changes fields that the device type classifies as requiring reconfiguration
- **THEN** the firmware calls the runtime `end` hook before applying the new config, applies the new config to the existing runtime object, and resets that runtime state machine to its initial `Idle` state

#### Scenario: Dependency changes relink and reset existing runtime
- **WHEN** a config mutation changes one or more dependency role/device-id links for the target runtime
- **THEN** the firmware applies the accepted config to the existing runtime object, refreshes dependency wiring, and resets the target runtime state machine to its initial `Idle` state

#### Scenario: Unchanged dependency payload is not structural
- **WHEN** a config mutation includes dependency links whose role/device-id pairs match the current runtime dependency links regardless of array order
- **THEN** the firmware does not treat the dependency payload as a dependency change

#### Scenario: Device is disabled
- **WHEN** a caller disables a dynamic device
- **THEN** the firmware applies the operation's persistence policy to the disabled state, stops runtime activity for that device, and reports the device as disabled without deleting its configuration

#### Scenario: Device is deleted
- **WHEN** a caller deletes a dynamic device that is allowed to be deleted
- **THEN** the firmware stops the runtime instance, removes the persisted record, clears all device-scoped data owned by the device, and no longer lists the device as active
