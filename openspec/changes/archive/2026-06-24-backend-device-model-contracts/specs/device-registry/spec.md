## ADDED Requirements

### Requirement: Registry uses canonical nested device records
The firmware SHALL load, mutate, and persist each dynamic device through a nested record/config boundary that keeps identity separate from persisted settings and runtime state.

#### Scenario: Loaded device reconstructs the canonical shape
- **WHEN** the registry loads a supported persisted device
- **THEN** it reconstructs the device as a nested record with identity in `record`, persisted settings in `config`, and live state in `runtime`

#### Scenario: Runtime-only changes stay out of config
- **WHEN** a device runtime changes status or output without changing persisted settings
- **THEN** the registry updates runtime state without duplicating `name`, `enabled`, or `deps` outside the config object

#### Scenario: Accepted config mutations update the record revision
- **WHEN** a device configuration mutation is accepted
- **THEN** the registry updates the device record revision metadata and keeps the persisted config payload separate from runtime output fields
