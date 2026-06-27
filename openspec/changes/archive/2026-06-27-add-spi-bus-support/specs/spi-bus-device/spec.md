## ADDED Requirements

### Requirement: SPI bus device is registered in the default catalog
The firmware SHALL register a dedicated `SpiBusDevice` type in the default device type catalog with a stable identity and backend lifecycle behavior suitable for shared SPI peripherals.

#### Scenario: SPI bus type is available by default
- **WHEN** the default device type catalog is created after SPI support exists
- **THEN** it includes `SpiBusDevice` with a stable `type_id`, current config version, disabled retained-state support, and periodic tick support suitable for bus lifecycle management

#### Scenario: SPI bus type has a distinct API identity
- **WHEN** the portal API serializes or creates an SPI bus device
- **THEN** it uses the dedicated SPI bus type name instead of reusing the I2C or OneWire bus identity

### Requirement: SPI bus config models shared wiring and host selection
The SPI bus runtime SHALL own the shared SPI controller configuration, including host selection and the common bus lines `SCK`, `MOSI`, and optional `MISO`.

#### Scenario: Common write-only wiring is accepted
- **WHEN** a valid SPI bus config provides a host selection, `SCK`, and `MOSI` without `MISO`
- **THEN** the firmware accepts the config as a valid SPI bus definition for write-only peripherals

#### Scenario: Read-capable wiring is accepted
- **WHEN** a valid SPI bus config provides a host selection, `SCK`, `MOSI`, and `MISO`
- **THEN** the firmware accepts the config as a valid SPI bus definition for read-capable peripherals that share that bus

#### Scenario: Invalid host selection is rejected
- **WHEN** a SPI bus config requests an unsupported host value
- **THEN** the firmware rejects the config as invalid

### Requirement: SPI bus config updates release and restart hardware
The SPI bus runtime SHALL release old bus hardware and reset its state machine when shared wiring or host configuration changes.

#### Scenario: SCK change restarts the bus
- **WHEN** an accepted SPI bus config update changes `SCK`
- **THEN** the firmware calls the SPI bus runtime end hook while the old bus config is still active, applies the new config to the existing runtime object, and resets the state machine to its initial idle state

#### Scenario: MOSI change restarts the bus
- **WHEN** an accepted SPI bus config update changes `MOSI`
- **THEN** the firmware calls the SPI bus runtime end hook while the old bus config is still active, applies the new config to the existing runtime object, and resets the state machine to its initial idle state

#### Scenario: MISO change restarts the bus
- **WHEN** an accepted SPI bus config update changes `MISO`
- **THEN** the firmware calls the SPI bus runtime end hook while the old bus config is still active, applies the new config to the existing runtime object, and resets the state machine to its initial idle state

#### Scenario: Host change restarts the bus
- **WHEN** an accepted SPI bus config update changes the SPI host selection
- **THEN** the firmware calls the SPI bus runtime end hook while the old bus config is still active, applies the new config to the existing runtime object, and resets the state machine to its initial idle state

#### Scenario: Invalid SPI bus config is rejected before cleanup
- **WHEN** a SPI bus config update is invalid
- **THEN** the firmware rejects the update before calling end, changing runtime config, resetting the state machine, or changing persistence state

### Requirement: SPI devices are selected by chip-select, not by address
The firmware SHALL treat each dependent SPI device as owning its own `CS` pin and SHALL not model SPI dependency identity as a bus address.

#### Scenario: Chip-select is a device-level identity
- **WHEN** a dependent SPI device is created or updated
- **THEN** its `CS` pin is stored with the device-specific configuration rather than in the shared SPI bus config

#### Scenario: Duplicate chip-select is rejected
- **WHEN** two dependent SPI devices attached to the same SPI bus use the same `CS` pin
- **THEN** the firmware rejects the dependency mutation and leaves the relationship graph unchanged

#### Scenario: No address is persisted for SPI selection
- **WHEN** an SPI device is serialized or transferred between registry records
- **THEN** the firmware preserves device selection through the `CS` pin and does not persist any bus address field

### Requirement: SPI bus provides exclusive shared access
The SPI bus runtime SHALL expose a guarded shared transaction so dependent devices can reuse the same physical bus without overlapping transfers.

#### Scenario: Dependent acquires the SPI bus
- **WHEN** a dependent SPI device requests the bus while the SPI bus runtime is ready and no transaction is active
- **THEN** the firmware grants the dependent a live bus handle and marks the transaction as active until the dependent releases it

#### Scenario: Concurrent SPI transaction is rejected
- **WHEN** a second dependent SPI device requests the bus while another dependent still holds an active transaction
- **THEN** the firmware rejects the second request and leaves the active transaction unchanged

#### Scenario: Releasing the transaction allows reuse
- **WHEN** the active dependent releases its SPI bus transaction
- **THEN** the firmware clears the active transaction state and allows the next dependent to acquire the bus
