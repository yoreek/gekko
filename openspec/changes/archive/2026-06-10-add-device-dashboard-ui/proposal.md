## Why

The portal currently has the backend and SPA scaffold, but the main device management experience is still minimal. We need a focused dashboard workflow that makes the registry usable from the controller itself: browse devices, inspect full device state, and run the common actions without falling back to raw JSON or debug tools.

## What Changes

- Add a dedicated device dashboard view centered on device cards and a detail modal dialog.
- Show the common device fields for all device types: `device_id`, `type`, `name`, `status`, `enabled`, `registry_revision`, `config_revision`, and `pending_persistence`.
- Add inline device actions from the detail modal: rename, enable/disable, delete, and command execution.
- Render a typed device UI for `DummyDevice` first, while keeping the dashboard extensible for future device types through shared base fields and inherited type-specific views.
- Keep the dashboard compatible with live WebSocket updates and mock mode so the same UI can be tested without firmware.

## Capabilities

### New Capabilities
- `device-dashboard-ui`: Device list cards, modal detail view, shared base fields, typed `DummyDevice` rendering, and common device actions.

### Modified Capabilities
- `portal-web-app`: Extend the SPA behavior from general portal shell into a device management dashboard workflow.
- `portal-realtime-state`: Ensure dashboard UI state updates and action results remain consistent with realtime device updates.

## Impact

- Frontend: `frontend/` route structure, dashboard views, modal dialogs, typed device renderers, and action flows.
- API usage: existing `/api/devices` and `/api/devices/:id/command` endpoints become the primary interaction path for the dashboard.
- Realtime state: WebSocket updates must continue to refresh the list and modal without forcing reloads.
- Mock mode: the dashboard actions must continue to work against the localStorage-backed mock database.
