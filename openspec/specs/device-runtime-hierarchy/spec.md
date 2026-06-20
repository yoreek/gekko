#device - runtime - hierarchy Specification

## Purpose

Define the shared runtime base behavior used by dynamic firmware devices.

## Requirements

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

### Requirement: Runtime API uses dependency terminology
The runtime boundary SHALL use dependency/dependent naming rather than legacy relationship naming.

#### Scenario: Dependency runtime is assigned
- **WHEN** the registry wires a device dependency
- **THEN** it assigns the dependency runtime by role on the dependent runtime

#### Scenario: Dependent runtime is attached
- **WHEN** a dependency runtime is wired to a dependent runtime
- **THEN** the dependency runtime exposes the dependent through a live dependent runtime list

#### Scenario: Legacy dependency aliases are removed
- **WHEN** firmware code is updated for the dependency model
- **THEN** domain code no longer calls legacy dependency aliases

### Requirement: DummyDevice inherits from the base runtime
The firmware SHALL keep `DummyDevice` inherited from the reusable runtime base while preserving its stable type id, base configuration compatibility, and lifecycle behavior, without preserving old command or retained-output simulation behavior.

#### Scenario: DummyDevice descriptor stays stable
- **WHEN** the default device type registry is created
- **THEN** `DummyDevice` remains registered with stable `type_id = 1` and current config version

#### Scenario: DummyDevice descriptor exposes simple capabilities
- **WHEN** callers inspect the `DummyDevice` type descriptor
- **THEN** `DummyDevice` reports no command support and no retained-state support

#### Scenario: DummyDevice config remains compatible
- **WHEN** an existing `DummyDevice` binary config payload is loaded
- **THEN** the firmware decodes it using the existing supported Dummy config versions and creates the runtime successfully

#### Scenario: DummyDevice lifecycle remains available
- **WHEN** `DummyDevice` is started, disabled, reconfigured, dependency-blocked, or deleted
- **THEN** it reports lifecycle statuses through the base runtime lifecycle hooks without adding type-specific command simulation

### Requirement: Runtime base does not own type-specific config
The firmware SHALL keep device-type configuration parsing, validation, JSON conversion, and descriptor registration owned by the concrete device type rather than the generic runtime base.

#### Scenario: Concrete runtime owns config codec
- **WHEN** a device type defines a binary config payload
- **THEN** the concrete device type provides the encode, decode, validate, and JSON adapter behavior for that payload

#### Scenario: Base runtime avoids hardware assumptions
- **WHEN** a hardware-backed runtime inherits from the base runtime
- **THEN** the base runtime does not require GPIO, I2C, or any other hardware-specific dependency

### Requirement: Runtime capabilities support composite devices
The runtime boundary SHALL expose narrow optional capabilities for temperature readings and switch-like output control without requiring concrete class casts.

#### Scenario: Temperature sensor exposes reading capability
- **WHEN** a runtime represents a temperature sensor with a latest reading
- **THEN** it can provide the reading and reading status through the generic runtime boundary

#### Scenario: Switch runtime exposes output capability
- **WHEN** a runtime represents a switch-like device
- **THEN** it can report supported output states, current output state, and accept an internal output request with the provided `now`

#### Scenario: Thermostat rejects missing capability
- **WHEN** a thermostat dep role references a runtime that does not expose the required capability
- **THEN** runtime validation rejects the dep before thermostat control starts
