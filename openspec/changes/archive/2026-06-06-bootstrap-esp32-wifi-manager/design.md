## Context

This repository is starting from an OpenSpec-only state and needs an initial PlatformIO firmware foundation for ESP32. The baseline target is `esp32dev` on `espressif32@6.9.0`, 240 MHz CPU, 40 MHz flash, Arduino framework, and C++17. The project is expected to evolve toward an ESPHome-like firmware platform with more advanced modules, so the first implementation must avoid a one-off WiFi example structure.

The initial product behavior is a common ESP32 provisioning flow: first boot starts a temporary access point and HTTP configuration portal, the user scans networks and enters credentials, and the device persists configuration in NVS. Espressif-compatible mobile provisioning is also a first-phase requirement, not a later enhancement, because its stability with Arduino/PlatformIO must be tested before the architecture grows around the fallback HTTP portal.

## Goals / Non-Goals

**Goals:**

- Create a PlatformIO Arduino firmware baseline with C++17, explicit module boundaries, and reproducible build settings.
- Establish a cooperative non-blocking runtime model that uses short `loop()` ticks and the existing StateMachine library for multi-step flows.
- Provide WiFi station management, first-boot provisioning, AP fallback, scan, save, connect, and recovery behavior.
- Support and validate Espressif-compatible mobile provisioning early, using Arduino ESP32 provisioning APIs where possible and ESP-IDF APIs only behind adapter boundaries when needed.
- Store configuration in NVS with schema versioning, validation, default values, and future migrations.
- Define JSON as the external import/export format while keeping the persisted representation small and reliable for MCU constraints.
- Keep RAM, flash, blocking operations, and credential handling suitable for ESP32-class devices.

**Non-Goals:**

- Implement the full ESPHome-like automation/component model in this change.
- Require SD card hardware for the baseline boot or provisioning path.
- Build a production mobile application; only protocol compatibility and firmware-side support are in scope.
- Add cloud connectivity, TLS certificate management, signed production update infrastructure, cloud-managed firmware rollout, or device fleet management.

## Decisions

### Use Arduino under PlatformIO

The baseline will use PlatformIO with `framework = arduino` and the requested ESP32 board settings. Arduino is the better fit for this project because there is already existing Arduino-oriented code to reuse, the developer is comfortable with the model, and the expected feature set can be built with Arduino ESP32 libraries while still allowing selective ESP-IDF API usage where needed.

ESP-IDF-only was considered because it exposes provisioning, NVS, event loops, and partition control directly. It is not the baseline choice because it would slow reuse of existing Arduino code and raise the entry cost without a clear current need. The design still isolates platform calls so selected ESP-IDF APIs can be used behind adapters if Arduino wrappers are insufficient.

The initial `platformio.ini` should follow the existing proven project profile:

- shared `[env]` defaults for common `lib_deps`, debug macros, and C++17 flags
- `[env:esp32dev]` pinned to `espressif32@6.9.0`, `board = esp32dev`, Arduino framework, 240 MHz CPU, 40 MHz flash, and `toolchain-xtensa-esp32@12.2.0+20230208`
- `board_build.partitions = my_partitions.csv` and `board_build.filesystem = littlefs`
- `monitor_speed = 921600`, `upload_speed = 921600`, and `monitor_filters = esp32_exception_decoder`
- `[env:esp32dev_ota]` extending `esp32dev` with `upload_protocol = espota`

The WiFi manager baseline should keep mandatory dependencies small, but the project profile can preserve optional feature flags and libraries that match existing code reuse.

### Keep firmware as small C++ modules over Arduino APIs

The application will expose C++17 modules such as `App`, `ConfigStore`, `WifiManager`, `ProvisioningPortal`, and `MobileProvisioning`. The modules will use RAII for owned handles where practical, return explicit status/error values, and avoid heap-heavy patterns in boot-critical paths. Arduino APIs such as `WiFi`, `WebServer`, `DNSServer`, and `Preferences` remain inside adapter modules rather than being spread through all feature logic.

### Use cooperative state machines for non-blocking flow

The baseline will use a cooperative loop architecture. `loop()` should return quickly and delegate work to `App::tick()` and module-level `tick()` methods. Multi-step flows such as WiFi connection attempts, WiFi scanning, provisioning sessions, retry/backoff, portal status updates, and reset/recovery should be expressed as explicit state machines using the existing StateMachine library where it fits.

FreeRTOS tasks are available on ESP32, but they are not the baseline coordination model. They should be reserved for future modules that genuinely need concurrency, blocking driver isolation, or timing separation. This keeps the early firmware easier to reason about and matches existing project experience.

Typical runtime shape:

```text
loop()
  App::tick()
    ConfigStore::tick()
    WifiManager::tick()
    ProvisioningHub::tick()
    PortalServer::tick()
```

The state machines should use `millis()`-based deadlines and event flags rather than blocking `delay()` loops. Blocking calls from Arduino or ESP-IDF APIs should be wrapped so the calling flow remains bounded or explicitly documented as an acceptable short operation.

### Prefer asynchronous HTTP portal stack

The HTTP configuration portal should use the existing async web stack preference: `esp32async/ESPAsyncWebServer` with `AsyncTCP`. This fits the cooperative runtime model better than manually calling a blocking `WebServer::handleClient()` loop and aligns with the provided PlatformIO example. `DNSServer` can still be used for captive portal behavior if it remains compatible with the async HTTP server.

The portal should keep static assets minimal and serve them from embedded strings or LittleFS depending on size. Since the baseline already enables LittleFS and a custom partition table, the design can support a small filesystem-backed UI without making the provisioning flow depend on large assets.

### Include OTA as a baseline service

Firmware update support should be part of the baseline because it affects partition layout and recovery strategy from the beginning. The project should support two update paths:

- Development OTA through PlatformIO `espota`, using the existing `[env:esp32dev_ota]` environment.
- Web OTA through the device HTTP interface, using Arduino ESP32 `Update` APIs behind a `FirmwareUpdate` module.

Web OTA should be treated as an authenticated/local administrative operation once authentication exists. In the first baseline, it can be disabled by default or guarded by a build flag if the portal has no admin auth yet. The upload handler must check available OTA partition size before writing, stream chunks without buffering the full image in RAM, report progress/status, and reboot only after a completed update has been accepted.

Partition layout now matters early. If OTA is included, `my_partitions.csv` should reserve two app slots or otherwise explicitly document why Web OTA is not enabled for a given build. LittleFS remains useful for portal assets, but it must not consume space needed for safe firmware updates.

### Use Unity for practical automated tests

The project should use PlatformIO's Unity test runner for automated tests where the code can be tested deterministically. The highest-value automated targets are pure or lightly wrapped logic:

- configuration defaults and validation
- schema migration
- JSON import/export behavior
- StateMachine-driven transitions and timeout logic
- WiFi/provisioning coordinator decisions when hardware APIs are mocked or abstracted
- Web OTA request validation and size checks that do not require actually flashing firmware

Direct WiFi radio behavior, BLE/mobile provisioning compatibility, captive portal behavior on phone operating systems, and actual OTA flashing are not automated in the baseline because they require real hardware. These should be documented as repeatable manual integration checks rather than faked as host-only tests.

To keep Unity tests practical, application logic should avoid depending directly on global Arduino objects. Hardware-facing code should sit behind small interfaces or adapters so tests can exercise decision logic on the development machine without a connected microcontroller, real WiFi, NVS, HTTP server, or Update API state.

### Store canonical configuration in NVS with versioned keys

NVS will be the baseline persistence layer through Arduino ESP32 `Preferences` because WiFi credentials and small structured settings fit its intended use and do not require additional hardware. The configuration model will have a schema version and typed fields. Credentials will be stored in a dedicated namespace, while non-sensitive settings can use a separate namespace if useful for future export/reset behavior.

Alternatives considered:

- JSON text directly in NVS: simple and export-friendly, but less robust for partial updates and wastes limited NVS space as the configuration grows.
- Fully custom binary blob only: compact, but harder to inspect, migrate, and export.

The chosen approach is a typed internal model persisted through NVS keys or compact blobs, with JSON import/export at the boundary. The first implementation can store a single compact JSON blob only if the validation/migration layer treats it as an implementation detail and keeps room to migrate to typed keys.

### Use JSON only as interchange format

JSON will be supported for export, backup, debugging, and future API/UI interactions. The persisted representation must not expose callers to raw JSON mutation without validation. This keeps future compatibility while avoiding uncontrolled memory usage from large dynamic documents.

### Validate mobile provisioning before polishing HTTP portal

The device will support:

- SoftAP + HTTP portal for browser-based setup and local recovery.
- Espressif-compatible provisioning, preferably BLE provisioning through Arduino ESP32 provisioning support or a contained ESP-IDF bridge, for mobile-app setup.

The mobile provisioning path should be implemented and tested early with a real Android/iOS phone and Espressif-compatible app. This is the riskiest integration point in an Arduino baseline because compatibility can depend on Arduino core version, BLE stack behavior, provisioning security mode, and memory usage. The HTTP portal remains mandatory as a fallback and recovery path, but it should not hide mobile provisioning instability until late in the project.

Both transports will write into the same validated configuration model and connection workflow. If both are enabled, they must not race writes; a single provisioning coordinator owns the active session state.

### Treat SD card as optional extension storage

SD card support is not required for baseline configuration. The architecture should allow an optional storage provider later for logs, large assets, snapshots, backups, or advanced configuration bundles. NVS remains required for boot-critical settings so the device can start without SD hardware.

## Risks / Trade-offs

- Espressif mobile provisioning compatibility can vary by transport, app version, Arduino core version, and security mode -> Test it as an early milestone on real hardware/phone and isolate it behind `MobileProvisioning`.
- Captive portal behavior differs by OS and browser -> Provide deterministic AP, DNS redirect, and direct IP access; treat captive detection as best effort.
- NVS can wear or fill if configuration is rewritten too often -> Debounce writes, only persist changed values, keep blobs small, and expose reset/migration handling.
- JSON parsing can exhaust memory on MCU targets -> Bound input size, parse only at import/export boundaries, and validate before persisting.
- Cooperative code can accidentally become blocking as features are added -> Make state-machine based flow a baseline requirement and keep long waits, scans, retries, and provisioning sessions timeout-driven.
- First-boot provisioning can expose an open setup AP -> Use a temporary AP name derived from device identity, optional setup password support, and disable provisioning after successful setup unless explicitly reset.
- Future modules may need more partition space -> Define partition planning early and avoid consuming all flash with default partitions.
- Async web dependencies and optional feature libraries can increase flash/RAM usage -> Keep domain-specific libraries optional behind build flags and measure the baseline build before adding sensor/MQTT/Home Assistant modules.
- Web OTA can brick devices if partition size, upload validation, or reboot timing is wrong -> Check update size before writing, stream safely, surface errors, and keep serial/USB flashing as the early rollback path.
- OTA endpoints are sensitive before authentication exists -> Keep Web OTA behind a build flag or local/admin guard until access control is defined.
- Tests can become hard to run if all logic depends on Arduino globals -> Keep pure logic and hardware adapters separated so Unity can cover meaningful behavior without real devices for every case.

## Migration Plan

1. Add the PlatformIO Arduino project skeleton and confirm a minimal firmware builds for `esp32dev`.
2. Add the cooperative `App::tick()` lifecycle and integrate the existing StateMachine library.
3. Add NVS initialization and a versioned default configuration.
4. Add WiFi station connection using persisted credentials through a non-blocking state machine.
5. Add Espressif-compatible mobile provisioning behind a module boundary and test with a real phone.
6. Add SoftAP + HTTP provisioning and configuration save flow as the reliable fallback path.
7. Add OTA partition support, development OTA environment, and Web OTA module behind a safe guard.
8. Add JSON export/import functions and tests for validation/migration behavior.
9. Add Unity tests for deterministic logic and hardware/manual test plans for radio, mobile provisioning, captive portal, and OTA flashing.
10. Keep rollback simple during early development: erase NVS or use a factory reset action to return to first-boot provisioning, and retain serial flashing for failed OTA recovery.

## Open Questions

- Which provisioning security mode should be the default for Espressif mobile provisioning: proof-of-possession, no-pop for development, or QR-code based setup data?
- Should the baseline include a physical reset button requirement, or only a serial/HTTP reset action until hardware is defined?
- Which exact flash partition layout should be used once OTA and optional filesystem needs are decided?
- Which Arduino ESP32 provisioning API works reliably with `espressif32@6.9.0`, and does it require version pinning or direct ESP-IDF bridging?
- Should the first `my_partitions.csv` reserve OTA slots immediately, or keep a larger app partition until OTA requirements are firm?
