## MODIFIED Requirements

### Requirement: Runtime devices share a base lifecycle
The firmware SHALL provide a reusable base runtime class for dynamic devices so common lifecycle, status, dependency, dependent runtime wiring behavior, and shared runtime diagnostics coordination are implemented once.

#### Scenario: Base runtime implements common runtime API
- **WHEN** a dynamic device runtime inherits from the base runtime class
- **THEN** it receives common implementations for dependency runtime assignment, dependent runtime attach/detach, status reporting, and reconfigure/disable/delete requests

#### Scenario: Base runtime remains cooperative
- **WHEN** a derived runtime is ticked by the registry
- **THEN** the base runtime and derived runtime use the provided `now` timestamp and do not perform blocking waits

#### Scenario: Dependency readiness is reusable
- **WHEN** a derived runtime has a dependency runtime
- **THEN** the derived runtime can use shared dependency readiness behavior instead of duplicating dependency status checks

#### Scenario: Shared diagnostics coordination is reusable
- **WHEN** a bus runtime records or resets transient diagnostics
- **THEN** the runtime base or shared helper can coordinate the same runtime-only diagnostic counters and publish scheduling without duplicating the bookkeeping in each bus implementation

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

