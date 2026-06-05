## 1. PlatformIO Arduino Firmware Baseline

- [x] 1.1 Create `platformio.ini` with shared `[env]` defaults, the `esp32dev` Arduino environment, requested ESP32 clock settings, toolchain package, C++17 build flags, high-speed monitor/upload settings, LittleFS, custom partition file support, and ESP exception decoder monitor filter.
- [x] 1.2 Add Arduino application skeleton under `src/` with `setup()`, `loop()`, serial/logging initialization, WiFi service initialization, and deterministic startup ordering.
- [x] 1.3 Add module folders and headers for application lifecycle, configuration storage, WiFi manager, HTTP provisioning portal, and mobile provisioning.
- [x] 1.4 Add baseline partition configuration or document the default partition choice with room for future NVS, OTA, and optional filesystem decisions.
- [x] 1.5 Integrate the existing StateMachine library or adapter as the baseline pattern for multi-step non-blocking flows.
- [x] 1.6 Add `[env:esp32dev_ota]` extending the base ESP32 environment with `espota` upload settings.
- [x] 1.7 Declare baseline libraries for ArduinoJson, StreamUtils, ESPAsyncWebServer, AsyncTCP, and the existing local ReefDuino dependency where available, while keeping MQTT/sensor/Home Assistant libraries feature-gated.
- [x] 1.8 Add PlatformIO Unity test structure under `test/` for host/off-device logic tests that run without a connected microcontroller.
- [x] 1.9 Verify a clean PlatformIO build succeeds for the default ESP32 environment.

## 2. Device Configuration and NVS

- [x] 2.1 Define the versioned configuration model with device identity, WiFi credentials, provisioning settings, and future extension fields.
- [x] 2.2 Implement Preferences/NVS initialization and `ConfigStore` load/save behavior with typed defaults and explicit error handling.
- [x] 2.3 Implement validation for SSID, password, device name, schema version, and input size limits.
- [x] 2.4 Implement migration handling for supported older schema versions and recovery behavior for corrupt or unsupported data.
- [x] 2.5 Add reset behavior for clearing WiFi credentials without requiring a full flash erase.

## 3. WiFi Station and Provisioning Coordinator

- [x] 3.1 Implement WiFi station startup using stored credentials, Arduino ESP32 WiFi events, and a cooperative state machine for connection status.
- [x] 3.2 Implement retry limits and fallback timing through state-machine deadlines without deleting the last saved credentials.
- [x] 3.3 Implement provisioning coordinator state so HTTP and mobile provisioning share one validated save/connect flow.
- [x] 3.4 Implement setup AP startup with deterministic device-specific SSID and configurable security settings.
- [x] 3.5 Stop provisioning services after successful station connection unless fallback mode is explicitly active.

## 4. Espressif-Compatible Mobile Provisioning Spike

- [x] 4.1 Add Arduino ESP32 WiFi provisioning integration behind a `MobileProvisioning` module, using direct ESP-IDF bridging only if the Arduino API is insufficient.
- [x] 4.2 Configure provisioning transport, device identity, and security mode with development and production options separated.
- [x] 4.3 Route mobile-provided credentials through the same validation, persistence, and station connect workflow used by the HTTP portal.
- [x] 4.4 Coordinate mobile provisioning session success, failure, and timeout through the cooperative state-machine lifecycle.
- [x] 4.5 Verify provisioning with a real Android or iOS phone and an Espressif-compatible app.
- [x] 4.6 Record compatibility results, including Arduino core behavior, BLE/SoftAP transport behavior, security mode, memory impact, and any version pinning needed.

## 5. HTTP Configuration Portal

- [x] 5.1 Add ESPAsyncWebServer routes for portal page, WiFi scan API, configuration submit, connection status, and reset action.
- [x] 5.2 Implement bounded WiFi scan responses with SSID, RSSI, security type, and channel where available.
- [x] 5.3 Use asynchronous or bounded WiFi scan handling so portal requests do not block the cooperative loop for long periods.
- [x] 5.4 Implement credential submission validation and persistence through `ConfigStore`.
- [x] 5.5 Add best-effort captive portal DNS/redirect handling while keeping direct IP access reliable.
- [x] 5.6 Add a minimal UI asset strategy using embedded strings or LittleFS depending on asset size and partition budget.

## 6. JSON Import and Export

- [x] 6.1 Add JSON serialization for non-secret configuration fields with schema version and metadata.
- [x] 6.2 Add JSON import with bounded input size, parsing, validation, migration, and partial-field rejection behavior.
- [x] 6.3 Redact or omit WiFi passwords and future secrets from normal JSON export.
- [x] 6.4 Keep the persisted NVS representation hidden behind `ConfigStore` so it can change without breaking JSON compatibility.

## 7. Firmware Update

- [x] 7.1 Verify `[env:esp32dev_ota]` can perform development OTA uploads when the device is already connected to WiFi.
- [x] 7.2 Define or validate `my_partitions.csv` so app, OTA, NVS, and LittleFS partitions fit the baseline firmware strategy.
- [x] 7.3 Add a `FirmwareUpdate` module around Arduino ESP32 update APIs for Web OTA upload handling.
- [x] 7.4 Add guarded ESPAsyncWebServer routes for Web OTA upload, progress/status, success, and failure reporting.
- [x] 7.5 Stream Web OTA uploads in chunks, reject oversized images, and reboot only after successful finalization.
- [x] 7.6 Keep Web OTA disabled by default or behind an explicit development/admin guard until portal authentication is defined.

## 8. Verification

- [x] 8.1 Add Unity tests for configuration defaults, validation, migration, and JSON import/export where practical.
- [x] 8.2 Add Unity tests for StateMachine-driven WiFi retry/fallback and provisioning coordinator transitions using mocked time/events where practical.
- [x] 8.3 Add Unity tests for Web OTA validation logic such as disabled endpoint behavior, accepted size, oversized image rejection, and failure status mapping.
- [x] 8.4 Add documented manual integration steps for first boot, WiFi scan, credential save, station connection, failed connection fallback, and reset.
- [x] 8.5 Add documented manual integration steps for real phone mobile provisioning, captive portal behavior, and actual OTA flashing.
- [x] 8.6 Verify non-blocking behavior for connection retries, WiFi scans, provisioning sessions, portal handling, and Web OTA upload handling.
- [x] 8.7 Verify memory-sensitive paths enforce configured input limits for HTTP, JSON payloads, and Web OTA metadata.
- [x] 8.8 Run the PlatformIO build and host/off-device Unity tests, then record manual checks that still need an ESP32 device.
