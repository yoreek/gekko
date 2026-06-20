## ADDED Requirements

### Requirement: Thermostat config codec accepts canonical and alias temperature fields
The firmware SHALL accept thermostat config updates using either canonical fixed-point fields or Celsius aliases and SHALL preserve canonical fixed-point values in stored config and snapshots.

#### Scenario: Canonical fixed-point fields are accepted
- **WHEN** a thermostat config update includes `target_milli_celsius`, `min_safe_milli_celsius`, `max_safe_milli_celsius`, and `hysteresis_centi_celsius`
- **THEN** the firmware parses and persists those values without resetting them to defaults

#### Scenario: Celsius aliases are accepted
- **WHEN** a thermostat config update includes `target_celsius`, `min_safe_celsius`, `max_safe_celsius`, and `hysteresis_celsius`
- **THEN** the firmware converts them to the canonical fixed-point representation before storing the config

#### Scenario: Canonical values are exposed after update
- **WHEN** the API serializes a thermostat device after a successful config update
- **THEN** the returned config exposes the canonical fixed-point values that reflect the submitted settings
