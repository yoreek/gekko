## ADDED Requirements

### Requirement: DS18B20 device API contract
The portal API SHALL create, mutate, and serialize DS18B20 devices through the existing generic device registry endpoints.

#### Scenario: Create request includes parent and config
- **WHEN** a client creates a DS18B20 device through `POST /api/devices`
- **THEN** the request includes `type_id = 4`, common device fields, `has_parent = true`, `parent_device_id`, and a DS18B20 config object containing address, resolution, unit, poll period, report delta, and report policy

#### Scenario: Create rejects invalid parent or address
- **WHEN** a DS18B20 create request omits a compatible OneWire parent or contains an invalid DS18B20 address
- **THEN** the API returns the standard error envelope and does not create a partial registry record

#### Scenario: Update config can atomically update parent
- **WHEN** a client edits DS18B20 settings after creation
- **THEN** the SPA sends one `update_config` command to `POST /api/devices/:id/command` that can carry DS18B20 config and parent fields together, and the API validates the combined mutation before applying it

#### Scenario: Duplicate address is rejected
- **WHEN** a DS18B20 create or update request would duplicate an existing DS18B20 address on the same OneWire parent
- **THEN** the API returns the standard error envelope and does not change the registry

### Requirement: DS18B20 snapshot serialization
The portal API SHALL include DS18B20 persisted config, parent identity, lifecycle status, effective status, and latest temperature output in canonical device snapshots.

#### Scenario: Snapshot includes DS18B20 config
- **WHEN** the API serializes a DS18B20 device
- **THEN** the snapshot includes type `ds18b20_temperature_sensor`, parent fields, config address, resolution, unit, poll period, report delta, and report policy

#### Scenario: Snapshot includes valid temperature output
- **WHEN** the DS18B20 runtime has a valid reading
- **THEN** the snapshot includes `output.temperature` with numeric value, unit, unit symbol, measured timestamp, and valid state

#### Scenario: Snapshot tolerates missing reading
- **WHEN** the DS18B20 runtime has not produced a valid reading or the device is blocked
- **THEN** the snapshot remains valid JSON and includes `output.temperature.valid = false` for the unavailable reading

#### Scenario: Device list remains streamed
- **WHEN** the API streams the device registry list and DS18B20 devices are present
- **THEN** it serializes DS18B20 snapshots incrementally without concatenating the full device list into one temporary string

### Requirement: DS18B20 scan selection API support
The portal API SHALL expose enough OneWire scan data for DS18B20 selection while keeping scan execution on the parent OneWire bus device.

#### Scenario: Parent scan result remains generic
- **WHEN** the API serializes a OneWire bus scan result
- **THEN** it includes each scanned ROM address and family code so DS18B20 clients can filter family code `28`

#### Scenario: DS18B20 scan command targets parent
- **WHEN** the SPA requests a DS18B20 address scan
- **THEN** the API receives the existing `custom` command with payload `scan` for the selected OneWire parent device

#### Scenario: Non-DS18B20 scan candidate is not accepted as config
- **WHEN** a client submits an address from a non-`28` family scan result as DS18B20 config
- **THEN** the DS18B20 API adapter rejects the config even if the address came from a valid OneWire scan
