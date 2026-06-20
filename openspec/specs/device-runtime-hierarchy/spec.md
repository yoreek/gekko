# device-runtime-hierarchy Specification

## Purpose

Define the shared runtime base behavior used by dynamic firmware devices.

## Requirements

### Requirement: Runtime devices share a base lifecycle
The firmware SHALL provide a reusable base runtime class for dynamic devices so common lifecycle, status, parent dependency, and child runtime wiring behavior is implemented once.

#### Scenario: Base runtime implements common runtime API
- **WHEN** a dynamic device runtime inherits from the base runtime class
- **THEN** it receives common implementations for parent runtime assignment, child runtime attach/detach, status reporting, and reconfigure/disable/delete requests

#### Scenario: Base runtime remains cooperative
- **WHEN** a derived runtime is ticked by the registry
- **THEN** the base runtime and derived runtime use the provided `now` timestamp and do not perform blocking waits

#### Scenario: Parent dependency is reusable
- **WHEN** a derived runtime has a parent runtime
- **THEN** the derived runtime can use shared parent readiness behavior instead of duplicating parent status checks

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
