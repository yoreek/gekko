## MODIFIED Requirements

### Requirement: Runtime devices share a base lifecycle
The firmware SHALL provide a reusable base runtime class for dynamic devices so common lifecycle, status, dependency, and dependent runtime wiring behavior is implemented once.

#### Scenario: Base runtime implements common runtime API
- **WHEN** a dynamic device runtime inherits from the base runtime class
- **THEN** it receives common implementations for dependency runtime assignment, dependent runtime attach/detach, status reporting, and reconfigure/disable/delete requests

#### Scenario: Base runtime remains cooperative
- **WHEN** a derived runtime is ticked by the registry
- **THEN** the base runtime and derived runtime use the provided `now` timestamp and do not perform blocking waits

#### Scenario: Dependency readiness is reusable
- **WHEN** a derived runtime has a dependency runtime
- **THEN** the derived runtime can use shared dependency readiness behavior instead of duplicating dependency status checks

## ADDED Requirements

### Requirement: Runtime API uses dependency terminology
The runtime boundary SHALL use dependency/dependent naming rather than parent/child naming.

#### Scenario: Dependency runtime is assigned
- **WHEN** the registry wires a device dependency
- **THEN** it assigns the dependency runtime by role on the dependent runtime

#### Scenario: Dependent runtime is attached
- **WHEN** a dependency runtime is wired to a dependent runtime
- **THEN** the dependency runtime exposes the dependent through a live dependent runtime list

#### Scenario: Old parent methods are removed
- **WHEN** firmware code is updated for the dependency model
- **THEN** domain code no longer calls `parentRuntime()`, `setParentRuntime()`, or `childRuntimes()`
