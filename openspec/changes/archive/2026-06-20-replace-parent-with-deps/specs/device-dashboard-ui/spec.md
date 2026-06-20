## ADDED Requirements

### Requirement: Device UI uses dependency terminology
The SPA SHALL model and display device relationships as dependencies and dependents rather than parents and children.

#### Scenario: Device model includes deps
- **WHEN** the SPA normalizes a device snapshot
- **THEN** it stores `deps` and computed `hasDeps` and does not require `has_parent` or `parent_device_id`

#### Scenario: Labels use dependencies
- **WHEN** the UI displays relationship fields in English or Russian
- **THEN** labels use dependency wording rather than parent wording

#### Scenario: Mock data uses deps
- **WHEN** the SPA runs in mock mode
- **THEN** mock device records use `deps` and computed `has_deps` in the same shape as production snapshots

### Requirement: DS18B20 UI selects OneWire dependency
The SPA SHALL create and edit DS18B20 devices by selecting a OneWire bus dependency.

#### Scenario: Create sends deps
- **WHEN** the user creates a DS18B20 temperature sensor
- **THEN** the SPA sends a `deps` entry with role `onewire_bus` and the selected bus device id

#### Scenario: Edit sends deps with config
- **WHEN** the user edits DS18B20 settings or bus selection
- **THEN** the SPA sends one structured update containing the JSON config and `deps`
