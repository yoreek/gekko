## Purpose

Define the DS18B20 temperature sensor dynamic device type, its OneWire dependency relationship, runtime behavior, configuration contract, output reporting, and address discovery semantics.

## Requirements

### Requirement: DS18B20 temperature sensor device type
The firmware SHALL provide a `Ds18b20TemperatureSensorDevice` dynamic device type backed by a required OneWire dependency bus.

#### Scenario: Type descriptor is registered
- **WHEN** the default device type registry is created
- **THEN** it contains `Ds18b20TemperatureSensorDevice` with stable `type_id = 4`, current config format marker `DS18B20-1`, retained-state support disabled, dependent support disabled, 100 ms ticking enabled, and `compatibleDependencyTypes` containing only the OneWire bus type id

#### Scenario: Dependency relationship is required
- **WHEN** a caller creates a DS18B20 temperature sensor without a compatible OneWire dependency in `deps`
- **THEN** the firmware rejects the create request with an invalid relationship validation error and does not create the device

#### Scenario: Only OneWire dependency is accepted
- **WHEN** a caller creates or reassigns a DS18B20 temperature sensor to a device whose type is not `OneWireBusDevice`
- **THEN** the firmware rejects the mutation with an incompatible dependency type validation error

### Requirement: DS18B20 configuration contract
The firmware SHALL persist DS18B20 configuration as a bounded binary payload identified by the `DS18B20-1` format marker and containing enabled state, ROM address, resolution, output unit, poll period, report delta, and report policy.

#### Scenario: Valid configuration is accepted
- **WHEN** a DS18B20 config contains a valid `28` family OneWire ROM address with valid CRC, resolution 9 through 12, unit `celsius` or `fahrenheit`, poll period within bounds, a centi-Celsius report delta, and a boolean report-always flag
- **THEN** the firmware encodes and persists the config payload without storing the dependency id inside the type-specific config

#### Scenario: Invalid address is rejected
- **WHEN** the DS18B20 address is missing, malformed, not 16 hex characters, not family code `28`, or fails ROM CRC validation
- **THEN** the firmware rejects the create or update request before changing the registry

#### Scenario: Invalid sensor settings are rejected
- **WHEN** resolution, unit, poll period, report delta, or report policy is outside the supported range
- **THEN** the firmware rejects the create or update request with a clear config validation error

#### Scenario: Config survives reload
- **WHEN** a DS18B20 sensor is persisted and the firmware reloads the device registry
- **THEN** the sensor is restored with the same dependency relationship, address, resolution, unit, poll period, report delta, report policy, and config revision

### Requirement: DS18B20 cooperative runtime
The DS18B20 runtime SHALL use `StateMachine` states for initialization, conversion, reading, retry, reconfiguration, disabling, and deletion without blocking the main loop for conversion waits.

#### Scenario: Runtime initializes sensor resolution
- **WHEN** an enabled DS18B20 runtime starts with a ready OneWire dependency
- **THEN** it validates the configured ROM address, reads the scratchpad, writes the configured resolution when needed, avoids EEPROM copy in v1, and reaches its polling flow only after initialization succeeds

#### Scenario: Runtime waits after power up
- **WHEN** an enabled DS18B20 runtime starts or reinitializes after dependency bus setup
- **THEN** it waits a bounded startup interval using the App-provided `now` timestamp before the first scratchpad access

#### Scenario: Runtime waits by resolution
- **WHEN** a DS18B20 conversion is requested
- **THEN** the runtime sends an address-specific conversion command and waits using the App-provided `now` timestamp for the resolution-specific maximum conversion time before reading the scratchpad

#### Scenario: Runtime reads addressed scratchpad
- **WHEN** conversion wait is complete
- **THEN** the runtime reads the scratchpad using the configured ROM address, validates the scratchpad CRC, converts the raw value to internal milli-Celsius, and leaves other devices on the bus unaddressed

#### Scenario: Runtime handles read failure
- **WHEN** the dependency bus is unavailable, the device does not respond, the scratchpad CRC is invalid, or the parsed temperature is outside the DS18B20 range
- **THEN** the runtime marks the reading invalid, publishes unavailable output with `valid = false`, increments a consecutive error counter, and enters a bounded retry state

#### Scenario: Runtime faults recoverably after repeated failures
- **WHEN** read failures continue past the configured consecutive error threshold
- **THEN** the runtime may report `faulted` status for visibility while continuing retry attempts without requiring manual intervention

#### Scenario: Runtime auto-recovers
- **WHEN** a DS18B20 sensor starts responding again after previous read failures
- **THEN** the runtime clears the consecutive error count, publishes a valid temperature reading, and returns to normal polling automatically

#### Scenario: Runtime uses provided time
- **WHEN** the DS18B20 runtime evaluates poll periods, conversion waits, retries, or reconfiguration deadlines
- **THEN** it uses the `now` value passed to its tick handler and does not call `millis()` or `clock_.millis()` inside the domain handler

### Requirement: Temperature output reporting
The DS18B20 runtime SHALL expose the latest valid temperature as runtime output with value, unit, measured time, and validity state.

#### Scenario: Celsius output is serialized
- **WHEN** a DS18B20 sensor configured for Celsius has a valid reading
- **THEN** device snapshots include `runtime.output.temperature.value`, `runtime.output.temperature.unit = "celsius"`, `runtime.output.temperature.unitSymbol = "C"`, `runtime.output.temperature.measuredAtMs`, and `runtime.output.temperature.valid = true`

#### Scenario: Fahrenheit output is serialized
- **WHEN** a DS18B20 sensor configured for Fahrenheit has a valid reading
- **THEN** device snapshots include the Fahrenheit-converted value with `unit = "fahrenheit"` and `unitSymbol = "F"` while the runtime keeps its internal reading in Celsius

#### Scenario: Missing reading is serialized as invalid
- **WHEN** a DS18B20 runtime has no valid current reading after startup, reconfiguration, dependency blocking, or read failure
- **THEN** device snapshots include `runtime.output.temperature.valid = false` and do not require JSON `null` or `NaN` to represent the missing reading

#### Scenario: Changed value publishes output
- **WHEN** report-always is disabled and a completed reading differs from the previous valid internal reading by at least the configured report delta
- **THEN** the runtime marks its state dirty so REST and realtime snapshots expose the changed temperature

#### Scenario: Unchanged value stays quiet by default
- **WHEN** report-always is disabled and a completed reading differs from the previous valid internal reading by less than the configured report delta
- **THEN** the runtime keeps the reading available in snapshots but does not mark runtime state dirty solely because a poll completed

#### Scenario: Report always publishes each successful poll
- **WHEN** report-always is enabled and a reading succeeds
- **THEN** the runtime marks state dirty after every successful poll even when the measured temperature did not change

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

### Requirement: Multiple DS18B20 sensors on one bus
The firmware SHALL allow multiple DS18B20 dependent sensors on one OneWire bus without sharing scratchpad reads or blocking conversion waits.

#### Scenario: Duplicate address on one dependency is rejected
- **WHEN** a create, update, or dependency change would place two DS18B20 devices with the same ROM address under the same OneWire dependency
- **THEN** the firmware rejects the mutation and leaves the existing registry unchanged

#### Scenario: Independent addressed conversions are used
- **WHEN** multiple DS18B20 sensors share the same OneWire dependency
- **THEN** each dependent requests conversion for its configured address and later reads scratchpad data for that address only

#### Scenario: One sensor failure does not poison siblings
- **WHEN** one DS18B20 dependent fails to read or validate its scratchpad
- **THEN** sibling DS18B20 sensors on the same dependency continue their own state-machine flow and retain their own latest output state

#### Scenario: Dependency scan conflicts are avoided
- **WHEN** the OneWire dependency bus is actively scanning
- **THEN** DS18B20 dependent runtimes defer sensor transactions until the dependency bus is ready for dependent access

### Requirement: DS18B20 address discovery contract
The DS18B20 create and edit workflow SHALL support manual address entry and scan-based address selection filtered to DS18B20-family devices.

#### Scenario: Scan candidates are filtered
- **WHEN** a OneWire scan result contains devices from multiple family codes
- **THEN** DS18B20 selection exposes only candidates whose family code is `28` and whose ROM CRC is valid

#### Scenario: Manual address uses same validation
- **WHEN** a user enters a DS18B20 ROM address manually
- **THEN** the backend applies the same 16-hex, family code, and CRC validation as scan-selected addresses

#### Scenario: Scan is dependency scoped
- **WHEN** a user starts address scanning while creating or editing a DS18B20 sensor
- **THEN** the scan command targets the selected OneWire dependency device and does not scan unrelated OneWire bus devices
