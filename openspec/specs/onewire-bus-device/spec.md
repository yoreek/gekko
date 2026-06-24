## Purpose

Define the firmware contract for OneWire bus runtime configuration and dependency reinitialization.

## Requirements

### Requirement: OneWire bus config update impact
The OneWire bus runtime SHALL release old bus hardware and reset its existing state machine when bus hardware configuration changes.

#### Scenario: Bus pin change releases old hardware and restarts
- **WHEN** an accepted OneWire bus config update changes `gpioPin`
- **THEN** the firmware calls the OneWire bus runtime `end` hook while the old bus config is still active, applies the new config to the existing runtime object, and resets the state machine to its initial `Idle` state

#### Scenario: Bus pull-up change releases old hardware and restarts
- **WHEN** an accepted OneWire bus config update changes `internalPullup`
- **THEN** the firmware calls the OneWire bus runtime `end` hook while the old bus config is still active, applies the new config to the existing runtime object, and resets the state machine to its initial `Idle` state

#### Scenario: Bus reinitialization advances generation
- **WHEN** a OneWire bus runtime restarts after a pin or pull-up config change
- **THEN** it initializes the bus driver with the new config and advances its dependency generation so attached DS18B20 runtimes can reinitialize

#### Scenario: Invalid bus config is rejected before cleanup
- **WHEN** a OneWire bus config update is invalid
- **THEN** the firmware rejects the update before calling `end`, changing runtime config, resetting the state machine, or changing persistence state
