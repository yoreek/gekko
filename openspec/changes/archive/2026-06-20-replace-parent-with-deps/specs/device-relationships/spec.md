## MODIFIED Requirements

### Requirement: Typed device relationships
The firmware SHALL validate dependency/dependent relationships according to dependency roles and participating device types.

#### Scenario: Sensor is attached to compatible bus
- **WHEN** a caller creates a device whose type declares a required compatible `onewire_bus` dependency and the referenced bus exists with that type
- **THEN** the firmware stores the relationship in `deps` and includes the device in dependent queries for that bus

#### Scenario: Runtime dependency link is established
- **WHEN** a relationship is accepted and the device runtimes are instantiated
- **THEN** the dependent runtime receives a live pointer to its dependency runtime by role and the dependency runtime exposes the dependent runtime through its live dependent list

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
- **WHEN** a caller updates dependencies in a way that would create a cycle
- **THEN** the firmware rejects the mutation and preserves the previous valid graph

#### Scenario: Dependent limit is enforced
- **WHEN** a dependency device type declares a maximum number of dependents and a mutation would exceed that maximum
- **THEN** the firmware rejects the mutation before persisting it

### Requirement: Restrictive deletion with dependents
The firmware SHALL prevent accidental deletion of a device that still has dependent devices.

#### Scenario: Dependency delete is rejected
- **WHEN** a caller tries to delete a device that has dependent devices
- **THEN** the firmware rejects the delete request and reports the dependent device IDs

#### Scenario: Leaf delete is accepted
- **WHEN** a caller deletes a device that has no dependent devices
- **THEN** the firmware removes the device record after stopping its runtime instance and persists the updated registry

### Requirement: Dependency status propagation
The firmware SHALL propagate effective dependency status to dependent devices without rewriting dependent configuration.

#### Scenario: Dependency becomes unavailable
- **WHEN** a dependency device becomes disabled, faulted, missing, or deleting
- **THEN** the firmware reports each enabled dependent as disabled or dependency-blocked according to the dependency status

#### Scenario: Dependency recovers
- **WHEN** a dependency device returns to an available runtime status
- **THEN** the firmware allows enabled dependent devices to start or resume according to their own lifecycle state

#### Scenario: Dependent fault does not disable dependency
- **WHEN** a dependent device enters a fault state
- **THEN** the firmware preserves the dependency device enablement and runtime lifecycle unless the dependency type explicitly defines aggregate fault behavior

### Requirement: Relationship updates are atomic
The firmware SHALL apply dependency relationship changes atomically using the immediate registry persistence policy when dependency links change.

#### Scenario: Dependency reassignment succeeds
- **WHEN** a caller reassigns a dependency link from one compatible device to another compatible device
- **THEN** the firmware validates the full resulting graph, persists the updated relationship before returning success, rewires runtime dependency links, updates live dependent lists, and emits relationship/status events after the accepted mutation

#### Scenario: Dependency runtime is removed
- **WHEN** a dependent runtime is deleted or detached from a dependency
- **THEN** the firmware removes the dependent runtime from the old dependency's live dependent list and clears the dependent runtime's dependency pointer before the runtime object is destroyed or repurposed

#### Scenario: Persistence fails during relationship update
- **WHEN** a validated relationship mutation cannot be written to NVS
- **THEN** the firmware preserves the previous in-memory dependency graph and reports the mutation as failed
