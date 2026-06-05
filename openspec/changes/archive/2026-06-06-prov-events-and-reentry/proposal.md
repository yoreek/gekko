## Why

We need clearer visibility into provisioning lifecycle and a controlled way to re-enter BLE provisioning without wiping NVS. Right now it is hard to distinguish whether provisioning is actually active, and the only reliable recovery path is too coarse for normal field use.

## What Changes

- Add explicit log events for provisioning lifecycle transitions, including `PROV_START` and `PROV_END`.
- Add an admin-facing endpoint or button that re-enters BLE provisioning mode on demand without erasing stored WiFi/NVS data.
- Keep the existing WiFi portal and provisioning flow intact, but make provisioning re-entry an explicit control path.
- Preserve both provisioning transports, with BLE as the primary mobile path for this change.

## Capabilities

### New Capabilities
- `provisioning-control`: lifecycle logging for provisioning and manual re-entry into BLE provisioning without full reset.

### Modified Capabilities

## Impact

- Affects `MobileProvisioning`, `WifiManager`, `PortalServer`, and portal routes/UI.
- Adds a control path for provisioning restart without clearing NVS.
- Improves observability for mobile provisioning sessions and BLE discovery/debugging.

