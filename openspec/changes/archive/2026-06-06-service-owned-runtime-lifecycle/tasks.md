## 1. Mobile Provisioning Lifecycle

- [x] 1.1 Move no-credentials mobile provisioning startup from `App` into `MobileProvisioning` or `ProvisioningCoordinator` using explicit configuration/dependency inputs.
- [x] 1.2 Move WiFi fallback provisioning startup decisions from `App::tick()` into the provisioning lifecycle owner.
- [x] 1.3 Move provisioning re-entry handling from `App::tick()` into the provisioning lifecycle owner, preserving credential reset only for explicit re-entry.
- [x] 1.4 Move timeout recovery eligibility from `App::tick()` into the provisioning lifecycle owner so timed-out sessions restart without requiring a reboot.
- [x] 1.5 Preserve current BLE/SoftAP transport behavior, QR output behavior, session finish reasons, and credential submission flow.

## 2. OTA Lifecycle

- [x] 2.1 Give `ArduinoOtaService` explicit access to the device name and WiFi connectivity state through constructor or `begin(...)` dependencies.
- [x] 2.2 Move `!started && wifiManager.connected()` gating from `App` into `ArduinoOtaService`.
- [x] 2.3 Keep ArduinoOTA handling feature-gated by `WITH_ARDUINO_OTA` and safe to compile for host tests.

## 3. App Runtime Boundary

- [x] 3.1 Simplify `App::begin()` to platform initialization, config store begin/load, service begin calls, and initial deterministic service ticks only.
- [x] 3.2 Simplify `App::tick()` to computing `now` once and invoking service ticks in deterministic order.
- [x] 3.3 Remove App helper methods that encode service-specific runtime policy after their logic is owned by services.
- [x] 3.4 Keep service construction static and explicit without introducing a dynamic registry or broad `App` dependency.

## 4. Tests And Verification

- [x] 4.1 Add or update host tests for mobile provisioning no-credentials start, fallback start, re-entry credential reset, and timeout restart behavior where practical.
- [x] 4.2 Add or update host tests for OTA startup gating so OTA starts only after WiFi is connected.
- [x] 4.3 Run `scripts/test.sh`.
- [x] 4.4 Run PlatformIO firmware builds for `esp32dev` and `esp32dev_ota`.
- [x] 4.5 Manually verify on ESP32 that BLE provisioning, timeout restart, portal access, station reconnect, and development OTA still work.
