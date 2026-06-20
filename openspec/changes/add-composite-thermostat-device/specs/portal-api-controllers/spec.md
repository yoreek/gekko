## ADDED Requirements

### Requirement: Thermostat device API contract
The portal API SHALL create, mutate, and serialize thermostat devices through the existing generic device registry endpoints using the deps-shaped device contract.

#### Scenario: Create request includes deps and config
- **WHEN** a client creates a thermostat device through `POST /api/devices`
- **THEN** the request includes `type_id = 5`, common device fields, `temperature_sensor` and `switch` deps, and a thermostat config object

#### Scenario: Create rejects missing deps
- **WHEN** a thermostat create request omits the temperature sensor or switch dep
- **THEN** the API returns the standard error envelope and does not create a partial registry record

#### Scenario: Create rejects invalid config
- **WHEN** a thermostat create request contains an unsupported mode, invalid temperature bounds, invalid hysteresis, or invalid timing values
- **THEN** the API returns the standard error envelope and does not create a partial registry record

#### Scenario: Update config can atomically update deps
- **WHEN** a client edits thermostat settings after creation
- **THEN** one `update_config` command can carry thermostat config and deps together as structured JSON, and the API validates the combined mutation before applying it

### Requirement: Thermostat snapshot serialization
The portal API SHALL include thermostat persisted config, dep identity, lifecycle status, effective status, and latest control output in canonical device snapshots.

#### Scenario: Snapshot includes thermostat config
- **WHEN** the API serializes a thermostat device
- **THEN** the snapshot includes type `thermostat`, deps, mode, target temperature, hysteresis, safe min/max, check interval, sensor timeout, retry timeout, and minimum switch interval

#### Scenario: Snapshot includes control output
- **WHEN** the thermostat runtime has evaluated its control state
- **THEN** the snapshot includes desired switch state, actual switch state when available, latest temperature when valid, last check timestamp, and output status

#### Scenario: Snapshot tolerates unavailable temperature
- **WHEN** the thermostat runtime has no valid or fresh temperature reading
- **THEN** the snapshot remains valid JSON and marks the temperature output unavailable

#### Scenario: Device list remains streamed
- **WHEN** the API streams the device registry list and thermostat devices are present
- **THEN** it serializes thermostat snapshots incrementally without concatenating the full device list into one temporary string
