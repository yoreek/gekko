## MODIFIED Requirements

### Requirement: Thermostat device API contract
The portal API SHALL create, mutate, and serialize thermostat devices through the existing generic device registry endpoints using the deps-shaped device contract while distinguishing config-only edits from actual dependency reassignment.

#### Scenario: Create request includes deps and config
- **WHEN** a client creates a thermostat device through `POST /api/devices`
- **THEN** the request includes `type_id = 5`, common device fields, `temperature_sensor` and `switch` deps, and a thermostat config object containing mode, target, hysteresis, safe min/max, check interval, sensor timeout, retry timeout, and minimum switch interval

#### Scenario: Create rejects invalid dependency set
- **WHEN** a thermostat create request omits a compatible temperature sensor or switch dep
- **THEN** the API returns the standard error envelope and does not create a partial registry record

#### Scenario: Update config can atomically update deps
- **WHEN** a client edits thermostat settings after creation
- **THEN** the SPA sends one `update_config` command to `POST /api/devices/:id/command` that can carry thermostat config and dependency fields together as structured JSON, and the API validates the combined mutation before applying it

#### Scenario: Config-only update accepts unchanged deps
- **WHEN** a thermostat `update_config` request includes `deps` entries whose `temperature_sensor` and `switch` device ids match the current dependency links and changes only thermostat control config fields
- **THEN** the API accepts the update as a config-only mutation and the returned device snapshot reflects the updated config without requiring runtime object recreation

#### Scenario: Changed thermostat dependency relinks and resets
- **WHEN** a thermostat `update_config` request changes the `temperature_sensor` or `switch` dependency device id
- **THEN** the API applies the mutation as a dependency relink that validates dependencies, updates dependency links, and resets the thermostat runtime state machine to its initial `Idle` state

#### Scenario: Snapshot includes thermostat config and output
- **WHEN** the API serializes a thermostat device
- **THEN** the snapshot includes the thermostat config, dep links, lifecycle status, effective status, latest temperature output, desired switch output, actual switch output when available, and last check timestamp
