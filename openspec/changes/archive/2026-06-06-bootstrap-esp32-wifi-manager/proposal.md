## Why

The project needs a solid PlatformIO/ESP32 foundation before higher-level ESPHome-like features can be added. The first baseline capability is reliable WiFi provisioning and persistent configuration, because every future module depends on network access and device identity.

## What Changes

- Create a PlatformIO Arduino firmware baseline for `esp32dev` on `espressif32@6.9.0` using modern C++17.
- Base the initial PlatformIO profile on the proven existing Arduino ESP32 project style, including shared `[env]` settings, debug feature flags, LittleFS, custom partitions, high-speed serial/upload settings, and an OTA environment.
- Establish cooperative non-blocking runtime architecture using the existing StateMachine library for flows that need explicit state transitions.
- Add first-boot WiFi access point mode with an HTTP configuration UI.
- Support scanning nearby WiFi networks, selecting an SSID, entering credentials, and saving related device/network settings.
- Validate Espressif-compatible mobile-app WiFi provisioning early, including real phone testing, because this is a key integration risk for an Arduino-based firmware.
- Add baseline firmware update support using development OTA upload and a future-safe Web OTA path exposed through the device HTTP interface.
- Add PlatformIO Unity testing as far as practical, focusing automated coverage on pure logic and using hardware/manual test plans for WiFi, provisioning, and OTA behavior.
- Store configuration in NVS with versioning, validation, defaults, and a migration path for future settings.
- Define a configuration format strategy that can grow over time, including JSON import/export and a compact persisted representation when appropriate.
- Evaluate SD card support as optional future storage for larger configuration, logs, backups, or assets, without making it mandatory for the baseline.

## Capabilities

### New Capabilities

- `platformio-firmware-baseline`: PlatformIO Arduino project structure, ESP32 build settings, cooperative application lifecycle, logging, and module boundaries.
- `wifi-manager`: WiFi provisioning, captive/configuration portal, station connection flow, mobile provisioning compatibility, and credential persistence.
- `device-configuration`: Versioned configuration model stored in NVS with validation, defaults, migration, and JSON import/export.
- `firmware-update`: Development OTA upload, Web OTA upload, partition requirements, update validation, and recovery behavior.

### Modified Capabilities

None.

## Impact

- Adds initial firmware source tree, PlatformIO configuration, and build settings.
- Introduces dependencies for Arduino ESP32 WiFi, Preferences/NVS, HTTP server/captive portal, JSON serialization, and Espressif-compatible provisioning support.
- Reuses familiar Arduino ecosystem libraries where they fit the baseline, especially ArduinoJson, StreamUtils, ESPAsyncWebServer, AsyncTCP, Time/Timezone, MQTT-related libraries, and the local ReefDuino library as optional integration context.
- Establishes persistent configuration contracts that future device modules will extend.
- Affects flash/RAM budgeting, partition planning, boot-time behavior, and recovery/reset flows.
- Establishes the runtime pattern for future modules: short `loop()` ticks, explicit state machines, bounded work, and no long blocking waits in application flows.
- Requires partition planning that supports OTA update images and leaves enough room for LittleFS/NVS.
- Adds a test strategy based on Unity for configuration, state-machine, validation, migration, and serialization logic.
