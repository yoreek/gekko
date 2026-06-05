## 1. Provisioning lifecycle observability
- [x] 1.1 Emit explicit `PROV_START` when BLE provisioning starts.
- [x] 1.2 Emit explicit `PROV_END` when provisioning stops, succeeds, or fails.
- [x] 1.3 Keep lifecycle logging consistent across the existing BLE and SoftAP provisioning paths.

## 2. Manual BLE re-entry control
- [x] 2.1 Add a control path that re-enters BLE provisioning without erasing NVS-backed WiFi configuration.
- [x] 2.2 Ensure re-entry only resets the active provisioning session and related runtime state.
- [x] 2.3 Preserve the current portal and admin services while provisioning is restarted.

## 3. Portal integration
- [x] 3.1 Add an admin endpoint for triggering BLE provisioning re-entry.
- [x] 3.2 Add a portal button or equivalent UI action that calls the endpoint.
- [x] 3.3 Return a clear success or failure response so the UI can report provisioning state.

## 4. Verification
- [x] 4.1 Add tests for provisioning lifecycle transitions and logging triggers where practical in native tests.
- [x] 4.2 Validate that manual re-entry does not clear stored credentials.
- [x] 4.3 Validate that the portal remains available while provisioning is restarted.
