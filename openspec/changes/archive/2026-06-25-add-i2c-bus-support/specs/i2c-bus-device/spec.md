## ADDED Requirements

### Requirement: I2C bus runtime config update impact
The I2C bus runtime SHALL release its hardware, apply the accepted bus configuration, and reset its state machine when SDA, SCL, pull-up, or bus clock settings change.

#### Scenario: SDA change releases old hardware and restarts
- **WHEN** an accepted I2C bus config update changes the SDA pin
- **THEN** the firmware calls the I2C bus runtime `end` hook while the old bus config is still active, applies the new config to the existing runtime object, and resets the state machine to its initial `Idle` state

#### Scenario: SCL change releases old hardware and restarts
- **WHEN** an accepted I2C bus config update changes the SCL pin
- **THEN** the firmware calls the I2C bus runtime `end` hook while the old bus config is still active, applies the new config to the existing runtime object, and resets the state machine to its initial `Idle` state

#### Scenario: Clock change releases old hardware and restarts
- **WHEN** an accepted I2C bus config update changes the bus clock frequency
- **THEN** the firmware calls the I2C bus runtime `end` hook while the old bus config is still active, applies the new config to the existing runtime object, and resets the state machine to its initial `Idle` state

#### Scenario: Pull-up change releases old hardware and restarts
- **WHEN** an accepted I2C bus config update changes the internal pull-up setting
- **THEN** the firmware calls the I2C bus runtime `end` hook while the old bus config is still active, applies the new config to the existing runtime object, and resets the state machine to its initial `Idle` state

#### Scenario: Invalid bus config is rejected before cleanup
- **WHEN** an I2C bus config update is invalid
- **THEN** the firmware rejects the update before calling `end`, changing runtime config, resetting the state machine, or changing persistence state

### Requirement: I2C bus provides exclusive shared access
The I2C bus runtime SHALL expose a shared transaction guard so dependent devices can reuse the same physical bus without overlapping transfers.

#### Scenario: Dependency begins a bus transaction
- **WHEN** a dependent I2C device requests the bus while the I2C bus runtime is ready and no transaction is active
- **THEN** the firmware grants the dependent a live bus handle and marks the bus transaction as active until the dependent releases it

#### Scenario: Concurrent transaction is rejected
- **WHEN** a second dependent I2C device requests the bus while another dependent still holds an active transaction
- **THEN** the firmware rejects the second request and leaves the active transaction unchanged

#### Scenario: Releasing the transaction allows reuse
- **WHEN** the active dependent releases its I2C bus transaction
- **THEN** the firmware clears the active transaction state and allows the next dependent to acquire the bus

### Requirement: I2C dependent addresses are raw 7-bit values
The firmware SHALL treat each dependent I2C device as identified by a raw 7-bit I2C address and SHALL not store the read/write bit or 10-bit address state in dependency identity.

#### Scenario: Valid address is accepted
- **WHEN** a dependent I2C device supplies a raw address in the range `0x00` through `0x7F`
- **THEN** the firmware accepts the address as the device identity for that bus

#### Scenario: Address bit is not stored
- **WHEN** a dependent I2C device is serialized or transferred between registry records
- **THEN** the firmware preserves only the raw 7-bit address and does not persist a read/write bit

#### Scenario: Ten-bit identity is not stored
- **WHEN** a caller attempts to express a dependent I2C device using 10-bit address semantics
- **THEN** the firmware rejects the configuration as unsupported and keeps the dependency identity limited to 7-bit addressing

#### Scenario: Duplicate address is rejected
- **WHEN** a caller assigns a dependent I2C device the same raw address as another dependent already attached to the same bus
- **THEN** the firmware rejects the mutation and leaves the dependency graph unchanged

#### Scenario: Out-of-range address is rejected
- **WHEN** a dependent I2C device supplies an address greater than `0x7F`
- **THEN** the firmware rejects the configuration as invalid
