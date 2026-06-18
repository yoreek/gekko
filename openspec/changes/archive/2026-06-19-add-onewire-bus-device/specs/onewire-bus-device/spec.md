## ADDED Requirements

### Requirement: OneWire bus device type
The firmware SHALL provide a generic `OneWireBusDevice` dynamic device type with a stable numeric type id, versioned binary configuration, one configured bus data pin, and an optional internal pull-up setting.

#### Scenario: Type descriptor is registered
- **WHEN** the default device type registry is created
- **THEN** it contains `OneWireBusDevice` with stable `type_id = 3`, current config version `1`, command support enabled, retained-state support disabled, 100 ms ticking enabled, fast-loop ticking disabled, and runtime creation/validation callbacks

#### Scenario: Config stores bus pin and pull-up setting
- **WHEN** a OneWire bus device is created or updated
- **THEN** the firmware persists a bounded type-specific config payload containing the enabled flag, numeric data pin, and optional internal pull-up flag

#### Scenario: Pin is not model-validated in config
- **WHEN** a OneWire bus config contains a numeric pin value representable by the config format
- **THEN** firmware config validation accepts the value without rejecting it based on ESP32 model, board, strapping pin, or device-type pin policy

#### Scenario: Driver initialization failure faults runtime
- **WHEN** the concrete OneWire driver cannot initialize the configured pin or pull-up mode on the current hardware
- **THEN** the runtime reports `Faulted` without rewriting the persisted config payload

### Requirement: OneWire bus runtime remains cooperative
The firmware SHALL implement OneWire bus runtime lifecycle and scan flow with `DeviceRuntimeBase` and `StateMachine`, using the App-provided `now` timestamp for timing-aware work.

#### Scenario: Bus starts through explicit lifecycle states
- **WHEN** an enabled OneWire bus runtime starts
- **THEN** it configures the OneWire driver for the selected data pin and internal pull-up setting through explicit state-machine states before reporting `Ready`

#### Scenario: Bus uses provided time
- **WHEN** the OneWire bus runtime evaluates lifecycle transitions, scan progress, retries, or timeouts
- **THEN** it uses the `now` value passed to its tick handler and does not call `millis()` or `clock_.millis()` inside the domain handler

#### Scenario: Bus reconfiguration restarts hardware access
- **WHEN** the OneWire bus data pin or internal pull-up config changes for an enabled runtime
- **THEN** the runtime releases the old bus driver state, initializes the new pin, clears stale scan results, and moves back to `Ready` only after successful setup

#### Scenario: Bus disable clears active scan work
- **WHEN** a OneWire bus runtime is disabled or deleted
- **THEN** it cancels any active scan, clears scan-in-progress state, releases bus resources, and reports the appropriate lifecycle status

### Requirement: OneWire bus is sensor-agnostic
The firmware SHALL keep the OneWire bus device independent from DS18B20 and other concrete 1-Wire sensor implementations.

#### Scenario: Bus does not depend on DallasTemperature
- **WHEN** OneWire bus support is built
- **THEN** the bus runtime depends only on generic OneWire bus access and does not include or instantiate `DallasTemperature`

#### Scenario: Bus exposes generic device inventory
- **WHEN** a scan completes
- **THEN** the bus reports generic ROM addresses and family codes without interpreting them as temperature, memory, or other device-specific sensor types

### Requirement: OneWire scan workflow
The firmware SHALL support an explicit, bounded scan command that discovers devices on the configured OneWire bus and caches the latest scan result.

#### Scenario: Scan command starts discovery
- **WHEN** a caller sends the OneWire bus runtime a supported scan command while the bus is `Ready`
- **THEN** the runtime marks scan as in progress, clears the previous ready flag, and begins device discovery without blocking the main loop for the whole scan

#### Scenario: Scan advances on 100 ms cadence
- **WHEN** a OneWire scan is in progress
- **THEN** the runtime performs at most one OneWire search pass per `Tick100ms` cadence and relies on the driver for protocol-level microsecond timing within that pass

#### Scenario: Scan progress publishes runtime state
- **WHEN** `scan.in_progress` or `scan.ready` changes
- **THEN** the registry publishes `DeviceEventKind::StateChanged` for the bus device so websocket clients receive a refreshed type-specific snapshot

#### Scenario: Scan results are bounded
- **WHEN** the bus contains more devices than the firmware scan result capacity
- **THEN** the runtime records only up to the documented maximum device count, completes the scan, and exposes that the result was truncated

#### Scenario: Scan completes with devices
- **WHEN** the scan finds valid OneWire devices
- **THEN** the runtime marks scan ready, clears scan in progress, records the device count, and exposes the discovered ROM addresses

#### Scenario: Scan completes with no devices
- **WHEN** the scan finds no OneWire devices
- **THEN** the runtime marks scan ready, clears scan in progress, records device count `0`, and leaves the device list empty

#### Scenario: Concurrent scan is rejected
- **WHEN** a scan command is received while a previous scan is still in progress
- **THEN** the runtime rejects the command without clearing the active scan state or cached partial results

### Requirement: OneWire ROM address contract
The firmware SHALL represent OneWire device addresses as 64-bit ROM codes formatted as uppercase 16-character hexadecimal strings.

#### Scenario: Address bytes are formatted consistently
- **WHEN** the runtime serializes a discovered ROM address
- **THEN** it emits 8 bytes in bus order as 16 uppercase hex characters, where byte `0` is the family code, bytes `1..6` are the serial number, and byte `7` is the CRC byte

#### Scenario: CRC is validated during scan
- **WHEN** a scan candidate has an invalid OneWire CRC
- **THEN** the runtime excludes that candidate from the ready scan device list and records or reports the scan as containing an invalid candidate without exposing it as selectable

#### Scenario: Address parser accepts persisted sensor references
- **WHEN** future child sensor code parses a selected OneWire ROM address string
- **THEN** the shared parser accepts exactly 16 hex characters, rejects malformed input, and returns the 8-byte ROM address without allocating heap storage in hot paths

### Requirement: OneWire bus can be a parent device
The firmware SHALL allow OneWire bus devices to act as bounded parent devices for future child sensor runtimes.

#### Scenario: Descriptor allows children
- **WHEN** the OneWire bus descriptor is registered
- **THEN** it declares `canHaveChildren = true` and a bounded `maxChildren` value suitable for multiple sensors on one bus

#### Scenario: Parent runtime exposes readiness
- **WHEN** a future child runtime is attached to a OneWire bus runtime
- **THEN** the child can rely on the existing parent dependency behavior and only treat the bus as available when the bus runtime status is `Ready`
