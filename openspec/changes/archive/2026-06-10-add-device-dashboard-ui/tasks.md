## 1. Dashboard scaffold

- [x] 1.1 Add the device dashboard route/view as the primary operational screen in the portal SPA.
- [x] 1.2 Render the device registry as a card grid or list using the shared base device fields.
- [x] 1.3 Add a modal dialog that opens from a selected device card and shows the selected device details.
- [x] 1.4 Keep the dashboard layout usable on desktop and mobile widths without overlapping primary controls.

## 2. Shared device model and typed views

- [x] 2.1 Introduce a shared frontend device view model with the common fields: `device_id`, `type`, `name`, `status`, `enabled`, `registry_revision`, `config_revision`, and `pending_persistence`.
- [x] 2.2 Map REST and WebSocket payloads into the shared model so the dashboard list and modal use the same structure.
- [x] 2.3 Add a typed `DummyDevice` detail section that extends the shared base view model.
- [x] 2.4 Add a safe fallback view for unknown device types that still renders the shared base fields.

## 3. Device actions

- [x] 3.1 Implement rename flow from the device modal and refresh the selected device after success.
- [x] 3.2 Implement enable and disable actions from the device modal and reflect the returned state.
- [x] 3.3 Implement delete confirmation and removal flow from the device modal.
- [x] 3.4 Implement command submission from the device modal against `POST /api/devices/:id/command`.
- [x] 3.5 Surface REST validation and command errors in the modal without breaking the dashboard state.

## 4. Realtime and mock parity

- [x] 4.1 Reconcile device cards and the open modal from `/ws` messages when registry or status changes arrive.
- [x] 4.2 Refresh the selected device state when `registry_revision` or `config_revision` changes indicate stale modal data.
- [x] 4.3 Keep the dashboard action flows working in `mockMode` with the localStorage-backed JSON database.
- [x] 4.4 Preserve deterministic `?mockMode=1&mockReset=1` behavior for dashboard smoke testing.

## 5. Verification

- [x] 5.1 Add or update unit tests for the shared device model, typed `DummyDevice` view, and action mapping.
- [x] 5.2 Add or update Playwright smoke coverage for opening a device card, inspecting the modal, and exercising supported mock actions.
- [x] 5.3 Verify the frontend build still fits the LittleFS budget and produces deployable gzip assets.
- [x] 5.4 Verify the standard firmware and native test pipeline still passes after the dashboard UI changes.
