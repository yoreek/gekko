## MODIFIED Requirements

### Requirement: DS18B20 dependency reinitialization
The DS18B20 runtime SHALL reinitialize after its critical sensor identity/config changes or after its OneWire dependency bus is reconfigured, while non-critical reporting and scheduling changes apply to the existing runtime without full reinitialization.

#### Scenario: Sensor address change reinitializes runtime
- **WHEN** an accepted DS18B20 config update changes the configured ROM address
- **THEN** the runtime applies the new config to the existing runtime object and resets the state machine to its initial `Idle` state before performing another temperature conversion

#### Scenario: Sensor resolution change reinitializes runtime
- **WHEN** an accepted DS18B20 config update changes `resolution`
- **THEN** the runtime applies the new config to the existing runtime object and resets the state machine to its initial `Idle` state so sensor resolution configuration and conversion timing are rebuilt

#### Scenario: Dependency relationship change reinitializes runtime
- **WHEN** an accepted DS18B20 update changes the OneWire dependency device id
- **THEN** the registry relinks the dependency runtime pointer and resets the DS18B20 state machine to its initial `Idle` state

#### Scenario: Poll period update reschedules without reinitialization
- **WHEN** an accepted DS18B20 config update changes only `poll_ms`
- **THEN** the runtime applies the new config without resetting the state machine and sets the next poll deadline to `now + newPollMs`

#### Scenario: Reporting config update does not reinitialize runtime
- **WHEN** an accepted DS18B20 config update changes only `report_delta`, `report_always`, or `output_unit`
- **THEN** the runtime applies the new config without resetting the state machine

#### Scenario: Dependency bus generation change reinitializes runtime
- **WHEN** the dependency OneWire bus reinitializes due to pin or pull-up config changes
- **THEN** each attached DS18B20 dependent detects the dependency generation change and repeats sensor initialization before reporting a new valid reading

#### Scenario: Reinitialization clears current reading
- **WHEN** a DS18B20 runtime starts or reinitializes
- **THEN** it exposes unavailable temperature output with `valid = false` until a new reading succeeds

#### Scenario: Dependency disabled stops dependent work
- **WHEN** the dependency OneWire bus is disabled
- **THEN** the DS18B20 dependent stops requesting conversions or reading scratchpad data until the dependency is enabled and ready again
