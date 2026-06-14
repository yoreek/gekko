## ADDED Requirements

### Requirement: Device snapshots expose switch runtime output
The portal API SHALL expose current switch-like runtime output state in device JSON snapshots without storing that state in the device config object.

#### Scenario: GPIO switch snapshot includes runtime output
- **WHEN** the API serializes a `GpioSwitchDevice` with a live runtime
- **THEN** the device JSON includes `output.state` with one of `on`, `off`, or `disabled`

#### Scenario: Runtime output is separate from config
- **WHEN** the API serializes a switch-like device snapshot
- **THEN** current runtime output state appears under `output` and not under `config`

#### Scenario: Config states remain persisted settings
- **WHEN** the API serializes a GPIO switch config
- **THEN** `config.startup_state` and `config.safe_state` remain persisted configuration settings independent from `output.state`

#### Scenario: Realtime snapshot includes runtime output
- **WHEN** a GPIO switch output state changes and the portal publishes a generic device update message
- **THEN** the message payload includes the same runtime `output.state` shape used by REST device snapshots

#### Scenario: No switch-specific topic is required
- **WHEN** the frontend observes GPIO switch output changes
- **THEN** it can update from the generic device update payload without subscribing to a dedicated switch topic

### Requirement: Type adapters can serialize runtime fields
The portal API SHALL pass the optional live runtime pointer to type-specific device API adapters when serializing device JSON.

#### Scenario: Adapter receives runtime context
- **WHEN** the controller serializes a device record and a live runtime exists
- **THEN** it calls the matching type adapter with both the persisted record and the runtime pointer

#### Scenario: Controller stays type-agnostic
- **WHEN** the controller serializes GPIO switch runtime output
- **THEN** the type-specific adapter writes the switch-specific `output` object without the controller branching on GPIO switch or switch types

#### Scenario: Missing runtime is tolerated
- **WHEN** a device record has no live runtime during serialization
- **THEN** the adapter still writes persisted fields and omits unavailable runtime-only output fields
