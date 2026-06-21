## ADDED Requirements

### Requirement: Runtime config update hooks
The runtime boundary SHALL expose generic hooks for ending old runtime ownership, applying accepted config to the same runtime object, and resetting the existing state machine when the device type requires reconfiguration.

#### Scenario: Runtime end hook is generic
- **WHEN** a runtime object implements `IDeviceRuntime`
- **THEN** it exposes an `end` hook that receives the current tick timestamp and defaults to no-op for runtimes that do not need cleanup

#### Scenario: Config applies to existing runtime
- **WHEN** a validated config update is accepted for a runtime
- **THEN** the runtime can apply the new record and config blob to the existing object without requiring object recreation

#### Scenario: Reconfiguration resets to Idle
- **WHEN** a runtime config update requires state-machine reinitialization
- **THEN** the existing runtime state machine is reset to its initial `Idle` state and resumes through the normal cooperative startup flow on subsequent ticks

#### Scenario: Runtime update remains cooperative
- **WHEN** the registry calls runtime config update, end, or reset hooks
- **THEN** those hooks perform bounded immediate work only and do not block, delay, or perform retry loops inside the mutation path
