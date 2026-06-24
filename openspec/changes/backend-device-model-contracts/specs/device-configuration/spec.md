## MODIFIED Requirements

### Requirement: Canonical flat device record
The firmware SHALL define each dynamic device as a nested record with separate identity, persisted config, and runtime state sections.

#### Scenario: Defaults are loaded
- **WHEN** no configuration exists in NVS
- **THEN** the firmware creates an in-memory configuration using safe defaults and the current supported config format

#### Scenario: Existing configuration is loaded
- **WHEN** configuration exists in NVS with a supported config format
- **THEN** the firmware loads it, validates it, and exposes typed values to application modules

### Requirement: Configuration class hierarchy example
The firmware documentation SHALL show device configuration as a compact C++ inheritance chain that keeps shared persisted fields inside the base config layer instead of scattering them across unrelated descriptions.

#### Scenario: DS18B20 config derives from base config classes
- **WHEN** the documentation describes a DS18B20 configuration model
- **THEN** the example chain is:
  ```cpp
  class BaseDeviceConfig {};
  class BaseSensorConfig : public BaseDeviceConfig {};
  class BaseOneWireSensorConfig : public BaseSensorConfig {};
  class Ds18b20SensorConfig : public BaseOneWireSensorConfig {};
  ```
- **AND** `BaseDeviceConfig` owns the shared persisted fields `name`, `enabled`, and `deps`, `BaseOneWireSensorConfig` adds the OneWire-specific address, and `Ds18b20SensorConfig` adds the DS18B20-specific settings while record identity stays in the record wrapper

### Requirement: JSON import and export
The firmware SHALL support JSON as an external setup-bundle interchange format using nested device setup records.

#### Scenario: Configuration is exported
- **WHEN** a caller requests a configuration export
- **THEN** the firmware returns JSON containing the current non-secret configuration fields and metadata needed for future import

#### Scenario: Exported device records use record/config nesting
- **WHEN** the firmware exports a device setup transfer bundle
- **THEN** each device record contains nested `record` and `config` sections, includes `id`, `typeName`, and `configRevision` inside `record`, and does not duplicate those fields at the top level

#### Scenario: Configuration is imported
- **WHEN** a caller submits JSON configuration within the accepted size limit
- **THEN** the firmware parses, validates, migrates if needed, and persists only accepted fields from the nested device setup record

#### Scenario: Secret handling during export
- **WHEN** configuration contains WiFi passwords or other secrets
- **THEN** the firmware excludes or redacts those secrets from normal JSON export unless an explicit secure backup mode is implemented

#### Scenario: JSON behavior is covered by Unity tests
- **WHEN** JSON import or export behavior is changed
- **THEN** Unity tests cover valid import, invalid import rejection, size limits, version handling, and secret redaction
