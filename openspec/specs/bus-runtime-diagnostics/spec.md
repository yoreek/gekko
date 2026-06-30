## Purpose

Define the runtime-only diagnostic contract for supported I2C and SPI bus devices.

## Requirements

### Requirement: Bus runtime diagnostics stay in runtime state
The firmware SHALL expose bus diagnostics as nested runtime state and SHALL not persist diagnostics in device config blobs, setup bundles, or config migrations.

#### Scenario: Runtime snapshot includes diagnostics
- **WHEN** the firmware serializes an I2C or SPI bus device with live runtime state
- **THEN** the snapshot includes a nested `runtime.diagnostics` object for transient diagnostic counters

#### Scenario: Config updates do not alter diagnostics
- **WHEN** a bus device receives a valid config update
- **THEN** the firmware applies the new config without copying diagnostics into persisted config data or resetting them as part of config serialization

### Requirement: Bus diagnostics can be reset explicitly
The firmware SHALL provide an explicit diagnostics reset action for supported bus devices that clears transient counters without changing device config.

#### Scenario: Reset clears counters
- **WHEN** a user or API client requests a diagnostics reset on a supported bus device
- **THEN** the firmware clears `consecutiveErrors`, `lastErrorCode`, `lastErrorAtMs`, and `errorOps` in runtime state

#### Scenario: Reset does not reconfigure the bus
- **WHEN** diagnostics are reset
- **THEN** the firmware does not change the bus config, generation, or dependency wiring

#### Scenario: Reset publishes updated runtime
- **WHEN** diagnostics are reset successfully
- **THEN** the firmware publishes a runtime snapshot reflecting the cleared diagnostics state

### Requirement: Diagnostic-only updates are debounced
The firmware SHALL debounce diagnostic-only runtime updates so repeated low-level errors do not flood the realtime channel while the underlying counters still update immediately in memory.

#### Scenario: Repeated errors are coalesced
- **WHEN** multiple diagnostic-only errors occur within the debounce window
- **THEN** the firmware updates the in-memory counters immediately and publishes at most one consolidated runtime snapshot after the quiet period

#### Scenario: Important state changes publish immediately
- **WHEN** a bus status transition, scan completion, or explicit diagnostics reset changes visible runtime state
- **THEN** the firmware publishes the updated snapshot immediately instead of waiting for the debounce window

### Requirement: I2C scan is explicit and cooperative
The firmware SHALL provide an on-demand I2C scan flow that advances cooperatively and checks one address per tick instead of blocking the loop with a full sweep.

#### Scenario: Scan starts only on request
- **WHEN** a client explicitly requests an I2C bus scan
- **THEN** the firmware starts a cooperative scan session and does not perform repeated background scanning

#### Scenario: Scan advances one address per tick
- **WHEN** an I2C bus scan session is active
- **THEN** each cooperative tick advances the scan by at most one address and records whether the address acknowledged

#### Scenario: Scan uses transaction response
- **WHEN** the I2C scan probes an address
- **THEN** the firmware uses the I2C transaction response to decide whether that address is present

#### Scenario: Scan completion updates runtime
- **WHEN** the I2C scan reaches the end of the address range
- **THEN** the firmware stores the completed scan result in runtime state and publishes a snapshot

### Requirement: SPI bus uses the same diagnostics envelope without discovery
The firmware SHALL expose the same nested runtime diagnostics envelope for SPI bus devices but SHALL not add address discovery or scan behavior for SPI.

#### Scenario: SPI runtime includes diagnostics
- **WHEN** the firmware serializes a live SPI bus device
- **THEN** the snapshot includes `runtime.diagnostics` with the same counter fields used by other supported bus runtimes

#### Scenario: SPI does not scan addresses
- **WHEN** a client requests scan behavior on an SPI bus device
- **THEN** the firmware does not attempt address discovery because SPI selection is chip-select based, not address based
