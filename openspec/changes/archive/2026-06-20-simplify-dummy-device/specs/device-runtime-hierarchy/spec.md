## MODIFIED Requirements

### Requirement: DummyDevice inherits from the base runtime
The firmware SHALL keep `DummyDevice` inherited from the reusable runtime base while preserving its stable type id, base configuration compatibility, and lifecycle behavior, without preserving old command or retained-output simulation behavior.

#### Scenario: DummyDevice descriptor stays stable
- **WHEN** the default device type registry is created
- **THEN** `DummyDevice` remains registered with stable `type_id = 1` and current config version

#### Scenario: DummyDevice descriptor exposes simple capabilities
- **WHEN** the default device type registry exposes the Dummy descriptor
- **THEN** `DummyDevice` reports no command support and no retained-state support

#### Scenario: DummyDevice config remains compatible
- **WHEN** an existing `DummyDevice` binary config payload is loaded
- **THEN** the firmware decodes it using the supported base Dummy config layout and creates the runtime successfully

#### Scenario: DummyDevice lifecycle remains available
- **WHEN** `DummyDevice` is started, disabled, reconfigured, dependency-blocked, or deleted
- **THEN** it reports lifecycle statuses through the reusable runtime base and cooperative state machine
