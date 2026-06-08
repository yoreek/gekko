## 1. Platform WiFi Policy

- [x] 1.1 Disable SDK credential persistence and Arduino auto reconnect before starting WiFi.
- [x] 1.2 Remove `WiFi.begin()` without explicit credentials from setup AP startup.
- [x] 1.3 Keep station/AP/scan driver methods bounded and explicit.

## 2. WifiManager Ownership

- [x] 2.1 Inject `ConfigStore` into `WifiManager` so it can validate, persist, clear, and read credentials directly.
- [x] 2.2 Add direct `WifiManager` APIs for portal credential submit and BLE config-mode request.
- [x] 2.3 Move portal routes and `App` wiring off `ProvisioningCoordinator`.

## 3. BLE Config State Machine

- [x] 3.1 Move WiFi/provisioning event registration and debug logging into `WifiManager`.
- [x] 3.2 Add bounded event flags for `PROV_CRED_RECV`, `PROV_END`, and station disconnect details.
- [x] 3.3 Add BLE config states that start provisioning, stop on credentials or timeout, wait for `PROV_END`, then deinitialize.
- [x] 3.4 Save BLE-received credentials through `ConfigStore` and enter normal station connection flow.

## 4. Remove Old Flow Layers

- [x] 4.1 Remove `MobileProvisioning` from app wiring and build.
- [x] 4.2 Remove `ProvisioningCoordinator` from app wiring and build.
- [x] 4.3 Rename/reframe portal UI/API wording from provisioning re-entry to BLE config mode.

## 5. Verification

- [x] 5.1 Update native tests for direct `WifiManager` credential submit/config-mode behavior.
- [x] 5.2 Add tests for BLE config event flags and stop/deinit state ordering where practical.
- [x] 5.3 Run `scripts/test.sh` and fix formatting/static/native test failures.

## 6. Simplify WifiManager State Flow

- [x] 6.1 Document `Idle` as the only normal STA/AP runtime chooser.
- [x] 6.2 Simplify `Connecting`, `CheckConnection`, `RetryDelay`, `Connected`, and `SetupAp` so each state owns only its local action or wait condition.
- [x] 6.3 Return credential apply and BLE completion flows to `Idle` instead of jumping directly to `Connecting` or `SetupAp`.
- [x] 6.4 Update native tests for the `... -> Idle -> next mode` transition model.
