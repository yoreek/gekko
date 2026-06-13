## Why

Dashboard panels and device widget coordinates are currently stored in browser `localStorage`, so the layout is tied to one browser and can be lost or diverge across clients. The portal needs a firmware-backed persistence contract so panel order, panel names, active panel selection, and widget grid coordinates can be restored consistently after reloads and across devices.

## What Changes

- Add a dashboard layout REST API that loads and saves the complete panel layout document.
- Persist panel metadata, panel order, active panel id, and device widget coordinates on the firmware side.
- Keep the existing grid coordinate shape (`x`, `y`, `w`, `h`) so the current SPA layout model can move from local storage to API persistence without a frontend data-shape rewrite.
- Preserve local/default fallback behavior when no backend layout exists yet or stored layout data is invalid.
- Remove `localStorage` as the authoritative source for dashboard panels/layout while allowing it only as a temporary migration/cache fallback if needed.
- Ensure deleted devices are pruned from saved panel layouts before returning or after registry mutations.
- Allow the same device to appear on multiple panels while preventing duplicate widget entries for the same device within a single panel.
- Make the add-device selector exclude devices already present in the active panel.

## Capabilities

### New Capabilities
- `dashboard-layout-persistence`: Firmware-backed storage, REST API, validation, and recovery rules for dashboard panels and widget layout coordinates.

### Modified Capabilities
- `dashboard-contract`: Dashboard layout persistence changes from browser-local storage to backend-backed state while preserving grid coordinates.
- `panel-dashboard-ui`: Panel tabs, panel management, active panel, reset layout, and widget movement must load from and persist through the dashboard layout API.
- `portal-api-controllers`: Add dashboard layout REST routes using the shared controller behavior and bounded JSON validation.
- `portal-web-app`: Expand the SPA API surface to include dashboard layout persistence and update mock mode to emulate it.

## Impact

- Firmware: new dashboard layout storage/domain component, REST controller routes, bounded JSON parse/serialize, LittleFS or existing config persistence integration.
- SPA: panel store/API client changes, mock transport support, removal of layout authority from `localStorage`, migration/default-layout handling.
- Tests: native firmware tests for validation/storage/API behavior and frontend checks for load/save/reset/delete-device layout behavior.
- Storage: one bounded dashboard layout document stored on the controller; no new external service or network dependency.
