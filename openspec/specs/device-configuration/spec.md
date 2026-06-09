## Purpose

Define the versioned configuration and persistence model used by the firmware.
## Requirements
### Requirement: Versioned configuration model
The firmware SHALL define a versioned device configuration model that can be extended by future capabilities.

#### Scenario: Defaults are loaded
- **WHEN** no configuration exists in NVS
- **THEN** the firmware creates an in-memory configuration using safe defaults and the current schema version

#### Scenario: Existing configuration is loaded
- **WHEN** configuration exists in NVS with a supported schema version
- **THEN** the firmware loads it, validates it, and exposes typed values to application modules

### Requirement: NVS persistence
The firmware SHALL persist boot-critical configuration in NVS through Arduino ESP32 `Preferences` or an equivalent adapter.

#### Scenario: Valid configuration is saved
- **WHEN** a module saves valid configuration changes
- **THEN** the firmware writes the changes to NVS and confirms the write result before treating the changes as committed

#### Scenario: Invalid configuration is not saved
- **WHEN** configuration values fail validation
- **THEN** the firmware rejects the update and leaves the last valid persisted configuration unchanged

### Requirement: Configuration migration
The firmware SHALL support migrating stored configuration from older supported schema versions to the current schema version.

#### Scenario: Older supported schema is found
- **WHEN** NVS contains configuration with an older supported schema version
- **THEN** the firmware migrates it in memory, validates the migrated result, and persists the current schema version

#### Scenario: Unsupported schema is found
- **WHEN** NVS contains configuration with an unsupported or corrupt schema version
- **THEN** the firmware logs the issue and enters a recovery path that does not use unsafe configuration values

#### Scenario: Migration logic is covered by Unity tests
- **WHEN** configuration validation and migration logic is changed
- **THEN** Unity tests cover supported defaults, valid stored data, older supported schema versions, and corrupt or unsupported schema inputs

### Requirement: JSON import and export
The firmware SHALL support JSON as an external configuration interchange format.

#### Scenario: Configuration is exported
- **WHEN** a caller requests a configuration export
- **THEN** the firmware returns JSON containing the current non-secret configuration fields, schema version, and metadata needed for future import

#### Scenario: Configuration is imported
- **WHEN** a caller submits JSON configuration within the accepted size limit
- **THEN** the firmware parses, validates, migrates if needed, and persists only accepted fields

#### Scenario: Secret handling during export
- **WHEN** configuration contains WiFi passwords or other secrets
- **THEN** the firmware excludes or redacts those secrets from normal JSON export unless an explicit secure backup mode is implemented

#### Scenario: JSON behavior is covered by Unity tests
- **WHEN** JSON import or export behavior is changed
- **THEN** Unity tests cover valid import, invalid import rejection, size limits, version handling, and secret redaction

### Requirement: Extensible storage strategy
The firmware SHALL keep NVS as the required boot-critical storage and allow optional future storage backends for large non-critical data.

#### Scenario: SD card is not present
- **WHEN** the device boots without an SD card
- **THEN** all baseline WiFi and device configuration behavior still works using NVS

#### Scenario: Future storage backend is added
- **WHEN** a future capability adds SD card or filesystem-backed storage
- **THEN** boot-critical settings remain available in NVS and the optional backend is not required for provisioning recovery

### Requirement: Controller configuration remains separate from dynamic device registry
The firmware SHALL keep controller-level configuration separate from the dynamic device registry while allowing both to use NVS-backed persistence.

#### Scenario: Controller configuration is loaded
- **WHEN** the firmware loads controller-level configuration for identity, WiFi, provisioning, or firmware update behavior
- **THEN** the firmware does not require dynamic device registry data to be present or valid

#### Scenario: Dynamic registry changes
- **WHEN** a caller creates, updates, disables, or deletes a dynamic device
- **THEN** the firmware persists the dynamic device registry without changing controller-level configuration fields

#### Scenario: Registry storage format does not constrain controller config
- **WHEN** the dynamic device registry uses an index plus per-device records in NVS
- **THEN** controller-level configuration may continue using its existing typed NVS keys or other format selected for controller settings

#### Scenario: Controller configuration changes
- **WHEN** a caller changes controller-level configuration such as WiFi credentials or provisioning settings
- **THEN** the firmware persists those controller settings without rewriting dynamic device records unless an explicit registry migration is required

