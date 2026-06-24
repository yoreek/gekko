## ADDED Requirements

### Requirement: Frontend device records use canonical model shape
The SPA SHALL represent device records with a single canonical `record/config/runtime` shape after API and realtime boundary normalization.

#### Scenario: Device record separates identity, config, and runtime
- **WHEN** frontend code consumes a device record from REST, websocket, mock data, or store state
- **THEN** identity fields are read from `record`
- **AND** persisted settings are read from `config`
- **AND** live status, output, readings, and snapshots are read from `runtime`

#### Scenario: Persisted base fields stay in config
- **WHEN** a device model needs `name`, `enabled`, or `deps`
- **THEN** it reads them from `config`
- **AND** the record wrapper does not duplicate those fields

#### Scenario: Frontend contract uses type names
- **WHEN** the SPA creates, edits, routes, or renders a device by type
- **THEN** the public/domain model uses `typeName`
- **AND** numeric `typeId` is limited to temporary compatibility or internal UI lookup during migration

#### Scenario: Runtime and registry metadata remain separate
- **WHEN** the SPA normalizes a device record
- **THEN** runtime fields remain under `runtime`
- **AND** registry revision stays on the response/store envelope
- **AND** `pendingPersistence` is not exposed in the frontend device model

### Requirement: Frontend device-specific models use shared base config
The SPA SHALL model each device-specific config as an extension of the shared base device config.

#### Scenario: Device config includes base config once
- **WHEN** a device-specific config type is declared
- **THEN** it extends the shared base config containing `name`, `enabled`, and `deps`
- **AND** it only adds fields owned by that device family

#### Scenario: Device-specific fields use camelCase
- **WHEN** frontend code reads or writes device-specific config or runtime fields
- **THEN** it uses camelCase field names in the frontend domain model
- **AND** any legacy snake_case compatibility remains at API/realtime boundaries only

#### Scenario: Dependency selectors use config deps
- **WHEN** DS18B20 or thermostat forms select dependency devices
- **THEN** the selected dependency is represented through `config.deps`
- **AND** helper fields such as `dependency_device_id`, `temperature_sensor_device_id`, and `switch_device_id` do not become canonical persisted frontend config fields
