## MODIFIED Requirements

### Requirement: Typed device relationships
The firmware SHALL validate dependency/dependent relationships as a list of links according to dependency roles and participating device types.

#### Scenario: Sensor is attached to compatible bus
- **WHEN** a caller creates a device whose type declares a required compatible `onewire_bus` dependency and the referenced bus exists with that type
- **THEN** the firmware stores the relationship in `deps` and includes the device in dependent queries for that bus

#### Scenario: Runtime dependency links are established by list entry
- **WHEN** dependency relationships are accepted and the device runtimes are instantiated
- **THEN** the dependent runtime receives live dependency runtime pointers by dependency list index
- **AND** each dependency runtime exposes the dependent runtime through its live dependent list

#### Scenario: Repeated-role dependency links are accepted by the registry
- **WHEN** a mutation includes more than one dependency with the same role
- **THEN** the base registry validation treats each dependency as a separate list entry
- **AND** it does not reject the mutation solely because roles repeat

#### Scenario: Device validation enforces cardinality
- **WHEN** a device type requires a role to appear exactly once or within another device-specific count
- **THEN** that device type's config or adapter validation rejects mutations that violate the role cardinality rule

#### Scenario: Incompatible relationship is rejected
- **WHEN** a caller creates or updates a dependency relationship that the dependent type, dependency role, or dependency type does not support
- **THEN** the firmware rejects the mutation and leaves existing registry state unchanged

#### Scenario: Dependency relink replaces the full list
- **WHEN** an accepted mutation replaces a device's dependency list
- **THEN** the firmware detaches the dependent runtime from every old dependency runtime
- **AND** clears old dependency runtime pointers
- **AND** attaches the dependent runtime to every new dependency runtime by dependency list entry

#### Scenario: Self relationship is rejected
- **WHEN** a caller attempts to set a device as one of its own dependencies
- **THEN** the firmware rejects the mutation and leaves existing registry state unchanged

### Requirement: Restrictive deletion with dependents
The firmware SHALL prevent accidental deletion of a device that still has dependent devices, including devices referenced through repeated-role dependency links.

#### Scenario: Dependency delete is rejected
- **WHEN** a caller tries to delete a device that has dependent devices
- **THEN** the firmware rejects the delete request and reports the dependent device IDs

#### Scenario: Repeated-role dependency delete is rejected
- **WHEN** a caller tries to delete a device referenced by a display through a `metric_source` dependency
- **THEN** the firmware rejects the delete request and reports the display device ID as a dependent

#### Scenario: Leaf delete is accepted
- **WHEN** a caller deletes a device that has no dependent devices
- **THEN** the firmware removes the device record after stopping its runtime instance and persists the updated registry
