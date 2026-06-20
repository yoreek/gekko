## ADDED Requirements

### Requirement: Devices store dependency links
The firmware SHALL store device relationships as bounded dependency links from a device to the devices it depends on.

#### Scenario: Device has no dependencies
- **WHEN** a device record has an empty `deps` array
- **THEN** the API reports `has_deps = false` and the registry does not persist any separate has-deps flag

#### Scenario: Device has dependencies
- **WHEN** a device record has one or more dependency links
- **THEN** the API reports `has_deps = true` computed from the stored `deps` array

#### Scenario: Dependency link has a role
- **WHEN** a dependency link is stored or serialized
- **THEN** it includes a stable role and a non-zero dependency device id

#### Scenario: Dependency array is bounded
- **WHEN** a create or update request contains more dependency links than the firmware bound allows
- **THEN** the firmware rejects the mutation before changing memory or persisted state

### Requirement: Dependents are derived
The firmware SHALL treat dependents as reverse lookups derived from stored dependency links rather than persisted lists.

#### Scenario: Dependent ids are queried
- **WHEN** callers need devices that depend on a target device
- **THEN** the registry derives those ids by scanning stored `deps`

#### Scenario: Dependent runtime list is live only
- **WHEN** runtimes are instantiated
- **THEN** dependency runtimes expose live dependent runtime pointers for coordination, but those pointers are not persisted

#### Scenario: Dependency delete is rejected
- **WHEN** a caller tries to delete a device that has derived dependents
- **THEN** the registry rejects deletion and reports `dependent_device_ids`

### Requirement: Dependency roles are semantic
The system SHALL identify dependency meaning by role rather than array position.

#### Scenario: Dependency order changes
- **WHEN** a request sends dependency links in a different array order
- **THEN** validation and runtime wiring resolve links by role and preserve behavior

#### Scenario: Duplicate single role is rejected
- **WHEN** a device type permits only one dependency for a role and a request provides that role more than once
- **THEN** the registry rejects the mutation

#### Scenario: Runtime gets dependency by role
- **WHEN** a runtime needs a specific dependency
- **THEN** it retrieves the dependency runtime by role rather than by array index

## REMOVED Requirements

### Requirement: Stored has-parent flag
**Reason**: A boolean relationship flag duplicates the relationship array and cannot represent multiple dependencies.
**Migration**: Use computed `has_deps = deps.length > 0`.

### Requirement: Stored parent device id
**Reason**: One parent id cannot represent multi-dependency devices and uses misleading parent terminology.
**Migration**: Use `deps[]` links with role names and device ids.
