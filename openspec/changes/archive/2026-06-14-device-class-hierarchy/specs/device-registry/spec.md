## ADDED Requirements

### Requirement: Registry creates inherited runtime devices
The firmware SHALL allow `DeviceTypeDescriptor::createRuntime` factories to return runtime instances that inherit from shared base runtime classes while preserving the existing `IDeviceRuntime` registry boundary.

#### Scenario: Runtime factory returns derived runtime
- **WHEN** a supported device record is loaded or created
- **THEN** the registry accepts a runtime object returned as `std::unique_ptr<IDeviceRuntime>` even when the concrete class inherits through one or more base runtime classes

#### Scenario: Registry remains unaware of hardware class hierarchy
- **WHEN** the registry ticks, disables, deletes, or reconfigures a runtime
- **THEN** it calls the existing `IDeviceRuntime` API without depending on whether the runtime is Dummy, switch base, GPIO switch, or a future switch variant

#### Scenario: Parent child wiring works through inherited runtimes
- **WHEN** inherited runtime classes are created for parent or child devices
- **THEN** the registry wires parent and child runtime pointers through the existing `IDeviceRuntime` methods

### Requirement: Registry supports switch-like retained output state
The firmware SHALL support retained output state for switch-like runtimes without requiring switch output changes to rewrite the device configuration record.

#### Scenario: Switch runtime output changes
- **WHEN** a switch-like runtime changes its output state and restore-previous-state is enabled
- **THEN** the registry persistence flow can persist the latest state through retained-state storage using the device id

#### Scenario: Switch runtime restore disabled
- **WHEN** a switch-like runtime changes its output state and restore-previous-state is disabled
- **THEN** the registry persistence flow does not persist switch retained output state

#### Scenario: Switch runtime starts with retained state
- **WHEN** the registry creates a switch-like runtime that supports restore-previous-state
- **THEN** it loads retained state by device id and applies it before the runtime reaches Ready when the retained payload is valid
