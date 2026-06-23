## ADDED Requirements

### Requirement: Runtime output snapshots remain type-specific
The runtime layer SHALL expose device output through type-specific snapshots owned by the concrete device runtime so consumers do not depend on one generic bag of optional fields.

#### Scenario: Switch runtime exposes switch output
- **WHEN** a switch-like runtime reports output
- **THEN** the snapshot contains only switch-relevant state such as logical output and physical level fields

#### Scenario: Temperature runtime exposes temperature output
- **WHEN** a temperature sensor runtime reports output
- **THEN** the snapshot contains temperature-specific fields such as value, unit, and validity information

#### Scenario: Thermostat runtime exposes control output
- **WHEN** a thermostat runtime reports output
- **THEN** the snapshot contains thermostat control fields such as desired switch state, actual switch state, control status, and last check time

### Requirement: Runtime snapshot ownership follows the concrete device type
The firmware SHALL let each concrete runtime own the extraction and serialization of its output snapshot while the shared runtime base only provides common lifecycle wiring.

#### Scenario: Base runtime stays generic
- **WHEN** a new device type inherits from the runtime base
- **THEN** the base runtime does not force unrelated output fields onto the device type

#### Scenario: Concrete runtime serializes its own output
- **WHEN** a runtime snapshot is serialized for a device
- **THEN** the concrete runtime determines which output fields are present and how they are interpreted
