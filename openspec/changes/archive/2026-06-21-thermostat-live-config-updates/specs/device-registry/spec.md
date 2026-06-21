## MODIFIED Requirements

### Requirement: Device lifecycle status
The firmware SHALL expose lifecycle status for each dynamic device from creation through stable runtime configuration updates, runtime operation, disabling, fault handling, state-machine reset, and deletion.

#### Scenario: Device starts after creation
- **WHEN** a valid enabled device is created
- **THEN** the firmware moves the device through an explicit creation/start lifecycle and exposes its resulting runtime status

#### Scenario: Device configuration changes apply to existing runtime
- **WHEN** a caller updates a valid configuration for an existing device
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
- **THEN** the firmware stops the runtime instance, removes the persisted record, and no longer lists the device as active

## ADDED Requirements

### Requirement: Stable runtime config updates
The registry SHALL apply accepted typed configuration updates to existing runtime objects and SHALL use device-owned hooks to decide cleanup and state-machine reset behavior.

#### Scenario: Validation happens before runtime mutation
- **WHEN** a typed config update is malformed, invalid, or references invalid dependency relationships
- **THEN** the registry rejects the update before calling runtime cleanup, applying new config, relinking deps, resetting state machines, incrementing config revision, or changing persistence state

#### Scenario: Runtime end uses old config
- **WHEN** an accepted config update requires reconfiguration cleanup
- **THEN** the registry calls the runtime `end` hook before applying the new config so the runtime can release resources described by the old config

#### Scenario: Persistence failure rollback is out of scope
- **WHEN** a config update has passed validation and has been accepted by the runtime update flow
- **THEN** the firmware treats the in-memory runtime config as current and does not run device-specific rollback logic for storage persistence failures
