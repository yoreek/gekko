## Why

WiFi runtime, SoftAP access, and BLE credential entry currently compete for the same ESP32 WiFi stack through separate managers. This causes reconnect loops, provisioning lifecycle races, hidden SDK credential storage, and unclear ownership of `WiFi.*` calls.

## What Changes

- Make `WifiManager` the single owner of WiFi runtime flow, including station connection, AP mode, BLE configuration mode, credential persistence, and provisioning lifecycle decisions.
- Remove `MobileProvisioning` and `ProvisioningCoordinator` as separate flow-control layers.
- Keep BLE credential entry as a small transport component that reports received credentials to `WifiManager` and does not call `WiFi.*` or write configuration.
- Use `PROV_CRED_RECV` as the event that credentials were received, then stop provisioning from the cooperative state machine.
- Use `PROV_END` as the event that provisioning has stopped, then call `wifi_prov_mgr_deinit()` from the cooperative state machine.
- Disable SDK credential persistence and Arduino WiFi auto reconnect so the firmware-owned `ConfigStore` and `WifiManager` retry policy are the source of truth.
- Rename/reframe the portal BLE action from "re-enter provisioning" to "start BLE config mode".
- **BREAKING**: Remove the coordinator/provisioning APIs and route calls that depended on them; portal routes will call `WifiManager` directly.

## Capabilities

### New Capabilities
- `wifi-ble-config-mode`: BLE credential entry mode managed by `WifiManager` with explicit stop/deinit lifecycle.

### Modified Capabilities
- `wifi-manager`: Station/AP/provisioning ownership changes so all WiFi stack commands and credential persistence decisions are made by `WifiManager`.

## Impact

- Affected code: `src/wifi`, `src/platform/ArduinoWifiDriver.*`, `src/provisioning`, `src/portal`, `src/core/App.*`, and native state-machine tests.
- Affected APIs: portal provisioning re-entry endpoint becomes a WiFi BLE config-mode request; credential submission routes call `WifiManager` directly.
- Affected storage: SDK WiFi credential persistence is disabled; `ConfigStore` remains the persistent source of truth.
- Affected debugging: one shared WiFi/provisioning event handler in `WifiManager` logs STA and provisioning events and converts them into state-machine flags.
