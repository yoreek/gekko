## Purpose

Define the OTA and firmware update behavior for the project baseline.

## Requirements

### Requirement: Development OTA upload
The project SHALL provide a PlatformIO OTA upload environment for development firmware updates over the local network.

#### Scenario: OTA environment is configured
- **WHEN** a developer selects the OTA PlatformIO environment
- **THEN** it extends the base ESP32 Arduino environment and uses `espota` upload settings

#### Scenario: Serial flashing remains available
- **WHEN** OTA upload is unavailable or fails during development
- **THEN** the base serial upload environment remains available for recovery flashing

### Requirement: OTA-capable partition layout
The firmware SHALL use or document a partition layout that supports the selected firmware update strategy.

#### Scenario: OTA is enabled
- **WHEN** Web OTA or development OTA runtime update support is enabled
- **THEN** the partition table provides sufficient OTA app partition space for the built firmware image

#### Scenario: Filesystem is enabled
- **WHEN** LittleFS is enabled for portal assets or other data
- **THEN** the partition layout reserves filesystem space without preventing OTA updates from fitting

### Requirement: Web OTA upload
The firmware SHALL support a Web OTA update path through the HTTP interface when the feature is enabled.

#### Scenario: Firmware image is uploaded
- **WHEN** an authorized user uploads a firmware image through the Web OTA endpoint
- **THEN** the firmware streams the upload to the update partition without buffering the full image in RAM

#### Scenario: Firmware image is too large
- **WHEN** an uploaded firmware image exceeds the available update partition size
- **THEN** the firmware rejects the update before committing partial invalid firmware as bootable

#### Scenario: Upload completes successfully
- **WHEN** the firmware image upload completes and the update API accepts it
- **THEN** the firmware reports success and reboots only after the update is finalized

#### Scenario: Upload fails
- **WHEN** a Web OTA upload fails validation, transfer, or finalization
- **THEN** the firmware reports the error and continues running the current firmware

#### Scenario: Update validation is covered by Unity tests
- **WHEN** Web OTA validation logic is changed
- **THEN** Unity tests cover accepted sizes, oversized images, disabled endpoint behavior, and failure status mapping where practical

### Requirement: Update access guard
The firmware SHALL guard Web OTA access so firmware updates are not exposed as an unauthenticated public endpoint in normal operation.

#### Scenario: No admin authentication exists yet
- **WHEN** the baseline build does not include portal authentication
- **THEN** Web OTA is disabled by default or protected by an explicit development build flag

#### Scenario: Update endpoint is disabled
- **WHEN** Web OTA is disabled
- **THEN** firmware update routes are not available from the HTTP portal
