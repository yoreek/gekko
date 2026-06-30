## MODIFIED Requirements

### Requirement: Bus device snapshots expose runtime diagnostics
The portal API SHALL include nested runtime diagnostics in bus device snapshots without moving those counters into persisted config fields.

#### Scenario: I2C snapshot includes diagnostics
- **WHEN** the API serializes an I2C bus device with live runtime state
- **THEN** the snapshot includes `runtime.diagnostics` with the transient bus diagnostic counters

#### Scenario: SPI snapshot includes diagnostics
- **WHEN** the API serializes an SPI bus device with live runtime state
- **THEN** the snapshot includes `runtime.diagnostics` with the transient bus diagnostic counters

#### Scenario: Diagnostics remain runtime-only
- **WHEN** the API serializes a bus device snapshot
- **THEN** the diagnostics fields appear only under `runtime` and not under `config`

### Requirement: Supported bus diagnostics can be reset through a structured command
The portal API SHALL expose a structured diagnostics reset command for supported bus devices.

#### Scenario: Reset command is accepted
- **WHEN** a client sends `POST /api/devices/:id/command` with `command = "resetDiagnostics"` to a supported bus device
- **THEN** the controller accepts the command and clears that device's runtime diagnostics state

#### Scenario: Reset command does not alter config
- **WHEN** a diagnostics reset command succeeds
- **THEN** the controller does not mutate the persisted bus config or dependency graph

### Requirement: I2C scan remains a structured command
The portal API SHALL expose I2C scan as a named structured command that starts the existing cooperative scan behavior on the selected I2C bus device.

#### Scenario: I2C scan command starts scan
- **WHEN** a client sends `POST /api/devices/:id/command` with `command = "scan"` to a ready I2C bus device
- **THEN** the controller starts the cooperative scan flow without requiring a public payload string

#### Scenario: I2C scan does not run as a background keepalive
- **WHEN** the API has not received an explicit I2C scan command
- **THEN** the controller does not initiate background bus scanning on its own

