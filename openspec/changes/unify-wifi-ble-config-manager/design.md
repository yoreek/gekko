## Context

The ESP32 has one WiFi stack shared by station mode, SoftAP mode, scanning, Arduino auto reconnect, and Espressif provisioning. The current firmware lets `WifiManager`, `MobileProvisioning`, `ProvisioningCoordinator`, and the Arduino WiFi core all influence that shared stack, which makes lifecycle ordering hard to reason about.

The desired runtime model is simpler: `WifiManager` owns the WiFi state machine and credential persistence. BLE configuration is a temporary credential-entry mode that provides SSID/password to `WifiManager`, then `WifiManager` stops provisioning, deinitializes provisioning, saves credentials, and runs its normal station retry flow.

## Goals / Non-Goals

**Goals:**

- Make `WifiManager` the only firmware domain object that decides when WiFi station, SoftAP, retry, BLE config, and credential storage actions happen.
- Preserve the current minimal provisioning lifecycle ordering: call `wifi_prov_mgr_stop_provisioning()`, wait for `PROV_END`, then call `wifi_prov_mgr_deinit()`.
- Use a single WiFi/provisioning event handler for debug visibility and event-to-flag conversion.
- Disable hidden SDK credential persistence and Arduino auto reconnect so `ConfigStore` and `WifiManager` remain the source of truth.
- Keep portal HTTP credential entry and BLE credential entry on the same persistence and station retry path.

**Non-Goals:**

- Add portal authentication.
- Preserve the old `MobileProvisioning` and `ProvisioningCoordinator` APIs.
- Restore old WiFi credentials after a newly submitted password fails to connect.
- Implement a full custom mobile app protocol beyond the minimum needed to receive SSID/password.

## Decisions

1. `WifiManager` owns all WiFi state transitions.

   `WifiManager` will expose direct methods for portal routes, such as submitting WiFi credentials and requesting BLE config mode. `ProvisioningCoordinator` is removed because its only useful responsibility was forwarding requests between portal/provisioning and `WifiManager`.

2. BLE provisioning events become state-machine inputs, not direct actions.

   The shared event handler logs all relevant STA/provisioning events and sets bounded flags. For example, `ARDUINO_EVENT_PROV_CRED_RECV` copies SSID/password into fixed-size storage and sets `provCredentialsReceived_`; `ARDUINO_EVENT_PROV_END` sets `provStopped_`. The callback does not call `wifi_prov_mgr_stop_provisioning()`, `wifi_prov_mgr_deinit()`, `ConfigStore::saveWifiCredentials()`, or `WiFi.begin()`. Those actions happen only from `WifiManager::tick()`.

3. Provisioning shutdown is explicit and ordered.

   The BLE config flow uses states equivalent to:

   ```text
   StartBleConfig
     -> BleConfigRunning
     -> ApplyBleCredentials when credentials are received
     -> StopBleConfig
     -> WaitBleConfigStopped
     -> DeinitBleConfig
     -> Idle
   ```

   `ApplyBleCredentials` saves credentials through the firmware configuration path before provisioning is stopped. `StopBleConfig` calls `wifi_prov_mgr_stop_provisioning()`. `WaitBleConfigStopped` waits for the event handler to observe `ARDUINO_EVENT_PROV_END`. `DeinitBleConfig` calls `wifi_prov_mgr_deinit()` and returns to `Idle`.

4. `PROV_CRED_RECV` is the credential handoff point.

   The firmware intentionally does not wait for `PROV_CRED_SUCCESS` or `PROV_CRED_FAIL` as control events because connection validation belongs to `WifiManager`. Receiving credentials is enough to stop BLE config mode, save the new credentials, and run the normal station retry flow. Setup AP remains the normal configurable mode when no station credentials are stored.

5. SDK WiFi persistence and Arduino auto reconnect are disabled.

   `ArduinoWifiDriver::begin()` will set `WiFi.persistent(false)` and `WiFi.setAutoReconnect(false)` before WiFi mode or connection attempts. The driver will not use `WiFi.begin()` without explicit credentials because that reconnects using the SDK's current STA config.

6. Portal routes depend on `WifiManager` directly.

   The portal credential submit and BLE re-entry endpoints will be updated to call `WifiManager` directly. The BLE action will be treated as "start BLE config mode" rather than "re-enter provisioning".

7. The main WiFi runtime flow returns to `Idle` for new choices.

   `Idle` is the only state that chooses between station connection and setup AP:

   ```text
   Idle
     -> Connecting when credentials exist
     -> SetupAp otherwise
   ```

   `Connecting` only starts `beginStation(credentials)` and moves to `CheckConnection`. `CheckConnection` observes BLE config request, pending portal credentials, station success, or connection timeout. On timeout it increments `retryCount`, then goes to `RetryDelay` if retries remain or `SetupAp` if retries are exhausted. `RetryDelay` returns to `Idle` or exits early for BLE config. `Connected` returns to `Idle` on disconnect and exits to BLE config when requested. `SetupAp` starts AP on entry and remains in AP mode until BLE config or submitted credentials move it. Credential apply and BLE completion states also return to `Idle` after their action.

   Ordinary runtime states do not poll unrelated commands. External control methods store requests as bounded pending inputs; states that can be interrupted by BLE config or submitted credentials handle those requests explicitly. Hardware/config actions still happen in states on cooperative ticks.

## Risks / Trade-offs

- [Risk] Standard Espressif provisioning may still apply STA config internally before `PROV_CRED_RECV` is delivered. -> Mitigation: stop provisioning immediately after credentials are received and restore the normal driver policy before applying credentials through `WifiManager`.
- [Risk] Removing coordinator/provisioning classes touches portal, app wiring, and native tests. -> Mitigation: keep the first implementation focused on state ownership and existing route behavior, then clean up stale tests and APIs in the same change.
- [Risk] Disabling SDK persistence can break fallback to `WiFi.begin()` without arguments. -> Mitigation: forbid `WiFi.begin()` without credentials in the firmware driver and always connect using `ConfigStore` credentials.
- [Risk] BLE config while connected to STA can temporarily disturb WiFi if the provisioning manager changes WiFi mode. -> Mitigation: treat BLE config as a managed `WifiManager` mode and make all mode recovery explicit after provisioning deinit.

## Migration Plan

1. Move portal credential/config-mode requests to `WifiManager`.
2. Move WiFi/provisioning event registration and debug logging into `WifiManager`.
3. Replace `MobileProvisioning` lifecycle with `WifiManager` BLE config states.
4. Remove `MobileProvisioning` and `ProvisioningCoordinator` files and stale app wiring.
5. Disable SDK persistence/auto reconnect and remove `WiFi.begin()` without arguments from AP startup.
6. Update tests and run `scripts/test.sh`.

## Open Questions

- Whether the first implementation should keep using Espressif `wifi_prov_mgr` directly or keep the Arduino `WiFiProv` wrapper temporarily while state ownership is moved.
- Whether BLE config mode should remain available while STA is connected or should intentionally disconnect first on this hardware profile.
