# Manual Integration Checks

These checks require a real ESP32 device and are intentionally not part of the host Unity test baseline.

## First Boot and HTTP Provisioning

- Flash the base `esp32dev` environment over serial.
- Erase NVS before the first-boot check when stored credentials are present.
- Confirm the device starts a setup AP with the device-specific SSID suffix.
- Connect a phone or laptop to the setup AP and open the portal by direct IP.
- Confirm `/api/wifi/scan` returns a bounded list of nearby networks.
- Submit valid credentials and confirm the setup AP policy follows the WiFi state machine instead of being forced off by the station connect path.
- Submit invalid or oversized values and confirm the last valid configuration remains unchanged.

## Failed Station Connection Recovery

- Save credentials for an unavailable network.
- Reboot the device.
- Confirm station connection retries are timeout-driven.
- Confirm the setup AP and HTTP portal remain available without deleting saved credentials.

## BLE WiFi Config Mode

- Enable mobile provisioning in the build/configuration.
- Confirm BLE config mode starts only after an explicit portal/API request.
- Confirm BLE transport is used; WiFiProv SoftAP transport is intentionally not used because setup AP and HTTP portal are owned by the firmware.
- Use an Espressif-compatible Android or iOS provisioning app.
- Confirm the log records `PROV_CRED_RECV` when the app submits credentials.
- Confirm the next cooperative `WifiManager` ticks save credentials through `ConfigStore`, call `wifi_prov_mgr_stop_provisioning()`, observe `PROV_END`, call `wifi_prov_mgr_deinit()`, and start the normal station connection flow.
- Confirm a BLE config timeout stops and deinitializes provisioning without changing stored credentials.
- Record transport, security mode, phone OS, app version, discovery result, credential submission result, and memory/build-size impact.
- Confirm HTTP portal provisioning remains available when BLE config mode is disabled.

## Captive Portal

- Test direct IP access from Android, iOS, Windows, and macOS when available.
- Test common captive portal detection flows and record platforms where automatic captive popup does not appear.

## OTA

- OTA-enabled builds are not the default target for the current 4 MB ESP32 board in this project.
- For 4 MB hardware checks, use the no-OTA single-app layout first; keep OTA validation for boards with more flash headroom.
- Use `esp32dev` for routine firmware compile verification. `esp32dev_ota` only inherits `esp32dev` and changes `upload_protocol`/`upload_port`, so reserve it for actual OTA upload checks or changes to OTA upload settings.
- See [platformio-environments.md](platformio-environments.md) for the canonical environment contract.
- Connect the device to WiFi and verify PlatformIO `esp32dev_ota` upload.
- Enable Web OTA only in a guarded development/admin build.
- Upload a valid firmware image and confirm reboot after successful finalization.
- Upload an oversized image and confirm the current firmware keeps running.
- Interrupt an upload and confirm the current firmware keeps running.

## Frontend Deployment

- See [frontend-deployment.md](frontend-deployment.md) for the rule that `portal-spa/` build refreshes `data/`, while `pio run -t uploadfs` remains the explicit ESP32 filesystem upload step.

## Non-Blocking Runtime

- During WiFi retries, scans, provisioning sessions, portal requests, and Web OTA upload, confirm serial logs continue and the cooperative loop remains responsive.

## Portal Controllers

- When adding or changing portal REST behavior, follow [controller-ruleschain.md](controller-ruleschain.md) so new endpoints keep the shared `BaseController` and `RulesChain` contract.
- Validate that `OPTIONS` preflight still returns CORS headers, non-API paths still fall back correctly, and path-guard hooks reject invalid ids with the expected JSON envelope.

## Dynamic Device Registry

- See [device-registry-persistence.md](device-registry-persistence.md) for the NVS storage format, boot reset behavior, and record-before-index commit order.
- After changing the registry format version, flash without erasing NVS over a firmware that used the previous format and confirm the device still boots, the portal remains reachable, and `/api/devices` returns a valid empty or rebuilt registry instead of crashing.
- Create a OneWire bus on GPIO4, run the `scan` command, and confirm a connected DS18B20 appears with family code `28`, a valid 16-character ROM address, and no invalid CRC flag.
- Create a DS18B20 child device using the scan result and confirm the device reaches `ready` with `output.temperature.valid = true`.
