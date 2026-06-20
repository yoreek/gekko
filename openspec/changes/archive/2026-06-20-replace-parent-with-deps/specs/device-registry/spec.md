## MODIFIED Requirements

### Requirement: Parent child lifecycle propagation
The device registry SHALL propagate dependency lifecycle changes to dependent runtime availability and effective status in a way that distinguishes disabled dependencies from broken or transitional dependencies.

#### Scenario: Disabled dependency makes dependent effectively disabled
- **WHEN** a dependency device is disabled and another device depends on it
- **THEN** the dependent device stops runtime work and its `effective_status` is reported as `disabled`

#### Scenario: Non-ready dependency blocks dependent
- **WHEN** a dependency device is missing, faulted, deleting, reconfiguring, or otherwise not ready for reasons other than being disabled
- **THEN** the dependent device stops runtime work and its `effective_status` is reported as `dependency_blocked`

#### Scenario: Dependency reconfiguration cascades to dependents
- **WHEN** a dependency runtime accepts a config change that reinitializes dependency hardware
- **THEN** each attached dependent runtime is requested to reconfigure after the dependency relationship is refreshed

#### Scenario: Dependency deletion is rejected with dependents
- **WHEN** a caller tries to delete a device that still has dependent devices
- **THEN** the registry rejects deletion, leaves the dependency and dependents unchanged, and reports the dependent device ids

#### Scenario: Dependency compatibility is enforced
- **WHEN** a device descriptor declares compatible dependency roles and types
- **THEN** create and dependency reassignment mutations reject dependency devices whose type id is not compatible with that role

## ADDED Requirements

### Requirement: Registry persists dependency records
The firmware SHALL persist dependency links in each device record as bounded role/device-id entries.

#### Scenario: Record stores deps
- **WHEN** a device with dependencies is persisted
- **THEN** its device record contains a bounded `deps` array and does not contain `hasParent` or `parentDeviceId`

#### Scenario: Registry builds dependents
- **WHEN** the registry loads or mutates records
- **THEN** it derives dependent relationships by scanning each record's `deps`

#### Scenario: Has deps is computed
- **WHEN** a registry snapshot is serialized
- **THEN** `has_deps` is computed from the number of dependency links and is not read from persistent storage
