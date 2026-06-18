## 1. WiFi connection flow

- [x] 1.1 Update the WiFi page so scanned access points can be selected from the results list while manual SSID/password entry remains available.
- [x] 1.2 Add a connect action that submits credentials through the existing WiFi configure API and keeps inline success or error feedback near the form.
- [x] 1.3 Restore the BLE config action on the WiFi page so the user can re-enter provisioning mode from the same surface.
- [x] 1.4 Add a reset-credentials action on the WiFi page that clears stored WiFi credentials through the WiFi API and keeps AP recovery explicit.

## 2. Mock and copy parity

- [x] 2.1 Keep the mock WiFi transport aligned with connect, BLE config, and reset-credentials flows so browser tests can exercise the same interaction.
- [x] 2.2 Add or update localized WiFi labels and messages for scan selection, manual entry, BLE config, reset, and connection feedback.

## 3. Firmware WiFi reset flow

- [x] 3.1 Add a WiFi manager credential-reset path that clears the runtime credential cache together with persisted WiFi credentials.
- [x] 3.2 Add guards in the station `CheckConnection` and `Connected` states so empty credentials return the machine through `Idle` and then into `SetupAp`.
- [x] 3.3 Expose a WiFi controller route for the reset action and keep the existing BLE config route available.

## 4. Verification

- [x] 4.1 Verify the WiFi page still scans nearby networks, allows manual refresh, and supports manual SSID/password entry after adding the extra actions.
- [x] 4.2 Verify selected scan results populate the connection field and the connect, BLE config, and reset actions use the expected API payloads.
- [x] 4.3 Run the relevant frontend and firmware checks after the WiFi flow update and confirm the SPA build still succeeds.
