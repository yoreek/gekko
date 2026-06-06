## MODIFIED Requirements

### Requirement: Development OTA upload
The project SHALL provide a PlatformIO OTA upload environment and a dependency-gated development OTA runtime listener for firmware updates over the local network.

#### Scenario: OTA environment is configured
- **WHEN** a developer selects the OTA PlatformIO environment
- **THEN** it extends the base ESP32 Arduino environment and uses `espota` upload settings

#### Scenario: OTA environment is upload-only for routine verification
- **WHEN** a developer verifies ordinary firmware code changes that do not alter OTA upload settings
- **THEN** the base `esp32dev` environment is the compile verification target and `esp32dev_ota` is not treated as a separate firmware build requirement

#### Scenario: Serial flashing remains available
- **WHEN** OTA upload is unavailable or fails during development
- **THEN** the base serial upload environment remains available for recovery flashing

#### Scenario: OTA runtime waits for usable WiFi/IP readiness
- **WHEN** development ArduinoOTA runtime support is enabled but the device does not yet have a usable WiFi IP address on station or setup AP
- **THEN** the OTA service remains in a dependency-wait state and does not call the OTA backend `begin()`

#### Scenario: OTA runtime starts from service tick
- **WHEN** station WiFi or setup AP becomes ready with a usable IP address during cooperative runtime
- **THEN** the OTA service starts the ArduinoOTA backend from its own tick-driven lifecycle without requiring `App` to gate startup

#### Scenario: OTA runtime handles WiFi changes
- **WHEN** the active WiFi/IP target disconnects or changes after the OTA backend has started
- **THEN** the OTA service stops the OTA backend, waits for a usable WiFi/IP target, and starts the backend again after readiness returns

#### Scenario: OTA runtime handles station IP changes
- **WHEN** the active OTA IP address changes after the OTA backend has started
- **THEN** the OTA service restarts the OTA backend so UDP and mDNS state are recreated for the new address
