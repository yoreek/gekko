## ADDED Requirements

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
The firmware SHALL refactor `DummyDevice` to inherit from the reusable runtime base while preserving its existing descriptor, configuration compatibility, lifecycle behavior, command behavior, retained state support, and tests.

#### Scenario: DummyDevice descriptor stays stable
- **WHEN** the default device type registry is created
- **THEN** `DummyDevice` remains registered with its existing stable type id and descriptor behavior

#### Scenario: DummyDevice config remains compatible
- **WHEN** an existing `DummyDevice` binary config payload is loaded
- **THEN** the firmware decodes it using the existing supported Dummy config versions and creates the runtime successfully

#### Scenario: DummyDevice lifecycle remains equivalent
- **WHEN** `DummyDevice` is started, disabled, reconfigured, faulted, dependency-blocked, or deleted
- **THEN** it reports the same lifecycle statuses as before the base-runtime refactor

#### Scenario: DummyDevice retained state remains supported
- **WHEN** `DummyDevice` starts with restore-previous-state enabled
- **THEN** it applies valid retained output state the same way it did before the base-runtime refactor

### Requirement: Runtime base does not own type-specific config
The firmware SHALL keep device-type configuration parsing, validation, JSON conversion, and descriptor registration owned by the concrete device type rather than the generic runtime base.

#### Scenario: Concrete runtime owns config codec
- **WHEN** a device type defines a binary config payload
- **THEN** the concrete device type provides the encode, decode, validate, and JSON adapter behavior for that payload

#### Scenario: Base runtime avoids hardware assumptions
- **WHEN** a hardware-backed runtime inherits from the base runtime
- **THEN** the base runtime does not require GPIO, I2C, or any other hardware-specific dependency
