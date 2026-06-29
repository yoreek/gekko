# device-relationships Specification

## Purpose
TBD - created by archiving change add-device-registry. Update Purpose after archive.
## Requirements
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

#### Scenario: Self relationship is rejected
- **WHEN** a caller attempts to set a device as one of its own dependencies
- **THEN** the firmware rejects the mutation and leaves existing registry state unchanged

### Requirement: Relationship graph integrity
The firmware SHALL preserve an acyclic dependency graph with valid device ID references.

#### Scenario: Missing dependency is rejected
- **WHEN** a caller assigns a dependency device ID that does not exist in the registry
- **THEN** the firmware rejects the relationship mutation

#### Scenario: Cycle is rejected
- **WHEN** a caller updates relationships in a way that would create a cycle
- **THEN** the firmware rejects the mutation and preserves the previous valid graph

#### Scenario: Dependent limit is enforced
- **WHEN** a dependency device type declares a maximum number of dependents and a mutation would exceed that maximum
- **THEN** the firmware rejects the mutation before persisting it

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

### Requirement: Dependency status propagation
The firmware SHALL propagate effective dependency status to dependent devices without rewriting dependent configuration.

#### Scenario: Dependency becomes unavailable
- **WHEN** a dependency device becomes disabled, faulted, missing, or deleting
- **THEN** the firmware reports each enabled dependent as dependency-blocked or equivalent unavailable status that references the dependency device ID

#### Scenario: Dependency recovers
- **WHEN** a dependency device returns to an available runtime status
- **THEN** the firmware allows enabled dependent devices to start or resume according to their own lifecycle state

#### Scenario: Dependent fault does not disable dependency
- **WHEN** a dependent device enters a fault state
- **THEN** the firmware preserves the dependency device enablement and runtime lifecycle unless the dependency type explicitly defines aggregate fault behavior

### Requirement: Relationship updates are atomic
The firmware SHALL apply dependency relationship changes atomically using the immediate registry persistence policy.

#### Scenario: Dependency reassignment succeeds
- **WHEN** a caller reassigns a dependent device from one compatible dependency to another compatible dependency
- **THEN** the firmware validates the full resulting graph, persists the updated relationship before returning success, rewires the dependent runtime to the new dependency runtime, updates the live dependent lists, and emits relationship/status events after the accepted mutation

#### Scenario: Dependency runtime is removed
- **WHEN** a dependent runtime is deleted or detached from its dependency
- **THEN** the firmware removes the dependent runtime from the old dependency's live dependent list and clears the dependent runtime's dependency pointer before the runtime object is destroyed or repurposed

#### Scenario: Dependency relink replaces the full list
- **WHEN** an accepted mutation replaces a device's dependency list
- **THEN** the firmware detaches the dependent runtime from every old dependency runtime
- **AND** clears old dependency runtime pointers
- **AND** attaches the dependent runtime to every new dependency runtime by dependency list entry

#### Scenario: Persistence fails during relationship update
- **WHEN** a validated relationship mutation cannot be written to NVS
- **THEN** the firmware preserves the previous in-memory relationship graph and reports the mutation as failed
