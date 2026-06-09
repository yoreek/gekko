# device-relationships Specification

## Purpose
TBD - created by archiving change add-device-registry. Update Purpose after archive.
## Requirements
### Requirement: Typed device relationships
The firmware SHALL validate parent/child relationships according to the participating device types.

#### Scenario: Sensor is attached to compatible bus
- **WHEN** a caller creates a child device whose type declares a required compatible parent and the referenced parent exists with that type
- **THEN** the firmware stores the relationship using the parent device ID and includes the child in relationship queries

#### Scenario: Runtime parent link is established
- **WHEN** a relationship is accepted and the device runtimes are instantiated
- **THEN** the child runtime receives a live pointer to its parent runtime and the parent runtime exposes the child runtime through its child list

#### Scenario: Incompatible relationship is rejected
- **WHEN** a caller creates or updates a device relationship that the child or parent type does not support
- **THEN** the firmware rejects the mutation and leaves existing registry state unchanged

#### Scenario: Self relationship is rejected
- **WHEN** a caller attempts to set a device as its own parent
- **THEN** the firmware rejects the mutation and leaves existing registry state unchanged

### Requirement: Relationship graph integrity
The firmware SHALL preserve an acyclic relationship graph with valid device ID references.

#### Scenario: Missing parent is rejected
- **WHEN** a caller assigns a parent device ID that does not exist in the registry
- **THEN** the firmware rejects the relationship mutation

#### Scenario: Cycle is rejected
- **WHEN** a caller updates relationships in a way that would create a cycle
- **THEN** the firmware rejects the mutation and preserves the previous valid graph

#### Scenario: Parent child limit is enforced
- **WHEN** a parent device type declares a maximum number of children and a mutation would exceed that maximum
- **THEN** the firmware rejects the mutation before persisting it

### Requirement: Restrictive deletion with dependents
The firmware SHALL prevent accidental deletion of a device that still has dependent child devices.

#### Scenario: Parent delete is rejected
- **WHEN** a caller tries to delete a device that has child devices
- **THEN** the firmware rejects the delete request and reports the dependent child device IDs

#### Scenario: Leaf delete is accepted
- **WHEN** a caller deletes a device that has no child devices
- **THEN** the firmware removes the device record after stopping its runtime instance and persists the updated registry

### Requirement: Dependency status propagation
The firmware SHALL propagate effective parent dependency status to child devices without rewriting child configuration.

#### Scenario: Parent becomes unavailable
- **WHEN** a parent device becomes disabled, faulted, missing, or deleting
- **THEN** the firmware reports each enabled child as dependency-blocked or equivalent unavailable status that references the parent device ID

#### Scenario: Parent recovers
- **WHEN** a parent device returns to an available runtime status
- **THEN** the firmware allows enabled child devices to start or resume according to their own lifecycle state

#### Scenario: Child fault does not disable parent
- **WHEN** a child device enters a fault state
- **THEN** the firmware preserves the parent device enablement and runtime lifecycle unless the parent type explicitly defines aggregate fault behavior

### Requirement: Relationship updates are atomic
The firmware SHALL apply relationship changes atomically using the immediate registry persistence policy.

#### Scenario: Parent reassignment succeeds
- **WHEN** a caller reassigns a child device from one compatible parent to another compatible parent
- **THEN** the firmware validates the full resulting graph, persists the updated relationship before returning success, rewires the child runtime to the new parent runtime, updates the parent child lists, and emits relationship/status events after the accepted mutation

#### Scenario: Parent runtime is removed
- **WHEN** a child runtime is deleted or detached from its parent
- **THEN** the firmware removes the child runtime from the old parent child list and clears the child runtime's parent pointer before the runtime object is destroyed or repurposed

#### Scenario: Persistence fails during relationship update
- **WHEN** a validated relationship mutation cannot be written to NVS
- **THEN** the firmware preserves the previous in-memory relationship graph and reports the mutation as failed

