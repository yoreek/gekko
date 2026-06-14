## Why

The backend can now create and control `GpioSwitchDevice`, but the SPA only has partial catalog/widget support. Users need a complete Vuetify UI to create, inspect, and command GPIO switch devices without hand-written API calls.

## What Changes

- Add type-specific GPIO switch create/edit fields in the devices UI.
- Add GPIO switch output controls for explicit `on`, `off`, and `disabled` commands.
- Add backend device JSON runtime output data for switch-like devices without mixing runtime state into config.
- Show GPIO switch configuration and current output state in the device detail surfaces and compact dashboard Power action.
- Extend the frontend mock API so local development and browser checks can exercise GPIO switch create/update/command flows.
- Keep the existing backend API contract and local icon registry; do not add external icon packages.

## Capabilities

### New Capabilities
- None.

### Modified Capabilities
- `device-type-catalog`: expose GPIO switch metadata needed by type-specific frontend forms and labels.
- `device-registry-table-ui`: support creating, editing, viewing, and commanding GPIO switch devices from the Devices page.
- `device-dashboard-ui`: render GPIO switch dashboard widgets through the device component registry while preserving compact dashboard layout behavior.
- `portal-api-controllers`: expose type-specific runtime output fields in device snapshots through the REST adapter boundary.

## Impact

- Affects `portal-spa` models, API contracts, mock handlers, device components, dashboard widgets, and localized labels.
- Backend device snapshots will add a runtime `output` object for switch-like devices; existing config fields and command endpoints remain unchanged.
- No new runtime dependencies unless an existing UI requirement cannot be met with Vuetify components; any proposed dependency must be agreed before installation.
