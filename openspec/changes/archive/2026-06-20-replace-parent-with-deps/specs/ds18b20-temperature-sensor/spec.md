## MODIFIED Requirements

### Requirement: DS18B20 device type
The firmware SHALL provide a `Ds18b20TemperatureSensorDevice` dynamic device type backed by a required OneWire bus dependency.

#### Scenario: DS18B20 type is registered
- **WHEN** the default device type registry is created
- **THEN** it contains `Ds18b20TemperatureSensorDevice` with stable `type_id = 4`, current config version `1`, retained-state support disabled, dependent support disabled, 100 ms ticking enabled, and a required `onewire_bus` dependency compatible with the OneWire bus type id

#### Scenario: DS18B20 requires dependency
- **WHEN** a caller creates a DS18B20 temperature sensor without a `onewire_bus` dependency in `deps`
- **THEN** the firmware rejects the create request before storing a partial device record

#### Scenario: DS18B20 rejects incompatible dependency
- **WHEN** a caller creates or updates a DS18B20 temperature sensor with a dependency that is not a OneWire bus
- **THEN** the firmware rejects the mutation and leaves the existing registry state unchanged

## ADDED Requirements

### Requirement: DS18B20 API uses deps
The portal API SHALL create, mutate, and serialize DS18B20 bus relationships through `deps`.

#### Scenario: Create request includes onewire dep
- **WHEN** a client creates a DS18B20 device through `POST /api/devices`
- **THEN** the request includes `type_id = 4`, common fields, a `deps` entry with role `onewire_bus`, and a DS18B20 config object

#### Scenario: Snapshot includes onewire dep
- **WHEN** the API serializes a DS18B20 device
- **THEN** the snapshot includes the selected OneWire bus under `deps` and does not include parent fields
