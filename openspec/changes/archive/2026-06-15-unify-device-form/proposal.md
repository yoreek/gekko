## Why

The portal currently has separate create and detail surfaces for devices, and the GPIO switch detail view shows primary runtime data mixed with secondary configuration values. This makes View/Edit/Create drift apart and makes common workflows harder to scan, especially when a user only needs the shared device identity fields and the most important type-specific fields.

## What Changes

- Introduce a single shared Device form structure that is reused for View, Edit, and Create flows.
- Render common fields first for every device type: name, type, and enabled state.
- Render type-specific fields after the common section once the device type is known or selected.
- Keep the default View mode readonly and compact, with the same header pattern for Dummy and GPIO switch devices.
- For GPIO switch devices, always show `GPIO pin` and current `Output state` in the primary type-specific section.
- Move secondary GPIO switch configuration fields under a collapsed-by-default `Config details` disclosure: `Startup state`, `Safe mode` / safe state, `Restore previous state`, and `Inverted`.
- Keep GPIO switch quick commands visible in View mode so users can send `On`, `Off`, and `Disabled` without entering Edit mode.
- Add inline `i` tooltip hints for `Startup state`, `Safe state`, and `Restore previous state` in View, Edit, and Create modes.
- In Create mode, show common fields first, then reveal the type-specific form after type selection, using default values for secondary GPIO switch configuration until the user expands and changes them.
- Centralize the light and dark theme color scheme so text, surfaces, controls, and status colors stay readable across the portal instead of relying on one-off component colors.
- Preserve existing mutation and command API contracts; no backend endpoint changes are required.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `device-dashboard-ui`: the device detail modal becomes a shared Device form surface for View/Edit/Create layout semantics, with common fields first and type-specific sections below.
- `device-registry-table-ui`: Devices table entry points must open the same shared Device form behavior as dashboard entry points.
- `portal-web-app`: the app shell and theme contract must use centralized semantic colors for light and dark modes so all visible UI elements remain readable against their backgrounds.

## Impact

- Affects the portal SPA device form/dialog components under `portal-spa/src/components/device` and `portal-spa/src/components/devices`.
- Affects the device UI registry so type-specific view/edit/create sections can be resolved through one form contract.
- Affects localization keys for common form labels, GPIO switch config details, and disclosure/action text.
- Affects frontend/component or smoke coverage for Dummy and GPIO switch View/Edit/Create flows.
- Does not require firmware runtime, device registry storage, or REST API changes.
