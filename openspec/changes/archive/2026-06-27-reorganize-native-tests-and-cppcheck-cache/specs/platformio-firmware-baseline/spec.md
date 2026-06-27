## MODIFIED Requirements

### Requirement: Host Unity test baseline
The project SHALL include a PlatformIO Unity testing baseline for deterministic firmware logic that can run without a connected microcontroller. Native Unity tests SHALL be organized into a small, fixed set of grouped suites so PlatformIO discovers fewer test targets, while preserving the same test coverage.

#### Scenario: Unity tests are runnable
- **WHEN** a developer runs the host/off-device Unity test command
- **THEN** Unity test suites compile and run for logic that does not require ESP32 hardware state

#### Scenario: Native suites are grouped
- **WHEN** PlatformIO scans the native test tree
- **THEN** it discovers grouped suites instead of one suite per test file, and each group has its own `test_main.cpp` entrypoint

#### Scenario: Hardware-dependent tests are separated
- **WHEN** a behavior requires real WiFi radio, BLE/mobile provisioning, captive portal client behavior, or actual OTA flashing
- **THEN** it is documented as manual integration coverage rather than mixed into host-only Unity tests

#### Scenario: Firmware builds from clean checkout
- **WHEN** a developer runs the PlatformIO build for the default ESP32 environment
- **THEN** the firmware compiles without requiring generated local files outside the repository

## ADDED Requirements

### Requirement: Cppcheck build-dir cache
The project SHALL configure `cppcheck` with an environment-specific persistent build-dir cache so repeated `pio check` runs can reuse cached analyzer state.

#### Scenario: Native check reuses cache
- **WHEN** a developer runs `pio check -e native` more than once without deleting the cache directory
- **THEN** `cppcheck` reuses the native build-dir cache under `.pio/cppcheck-cache/native`

#### Scenario: ESP32 check reuses cache
- **WHEN** a developer runs `pio check -e esp32dev` more than once without deleting the cache directory
- **THEN** `cppcheck` reuses the ESP32 build-dir cache under `.pio/cppcheck-cache/esp32dev`

#### Scenario: Cache directory is project-scoped
- **WHEN** the lint script prepares analyzer state
- **THEN** it creates the cache directories inside the project tree rather than depending on a global PlatformIO cache location

