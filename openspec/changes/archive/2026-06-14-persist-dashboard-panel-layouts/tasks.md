## 1. Firmware Layout Domain

- [x] 1.1 Add a bounded dashboard layout model with `schema_version`, `revision`, `active_panel_id`, panels, and widget coordinates.
- [x] 1.2 Implement validation for panel count limit of 8, unique panel ids/names, panel name length limit of 32 characters, active panel existence, widget coordinate values, duplicate widgets, schema version, and bounded payload limits.
- [x] 1.3 Implement deterministic default layout generation for empty, missing, invalid, or fully pruned stored layout state.
- [x] 1.4 Implement layout pruning against the device registry so deleted or unknown devices are removed from persisted layouts.
- [x] 1.5 Implement persistent load/save using the existing firmware storage pattern, with atomic replacement semantics and revision incrementing.

## 2. Firmware REST API

- [x] 2.1 Add `GET /api/dashboard/layout` using the shared REST controller success envelope with `revision` and `layout`.
- [x] 2.2 Add `PUT /api/dashboard/layout` using bounded JSON body parsing, validation, normalized atomic save, and revision response.
- [x] 2.3 Add `OPTIONS` and unsupported-method handling for `/api/dashboard/layout` through existing shared controller behavior.
- [x] 2.4 Ensure `/api/dashboard/layout` is routed as an API endpoint and is not swallowed by SPA fallback.
- [x] 2.5 Integrate layout pruning with device deletion or layout load so stale widget references are not returned.

## 3. SPA API And State

- [x] 3.1 Add a typed dashboard layout API client for `GET` and `PUT /api/dashboard/layout`.
- [x] 3.2 Update the panel/dashboard store to load panel order, panel names, active panel id, and widget coordinates from the backend layout API.
- [x] 3.3 Update panel reorder, rename, delete, active panel selection, widget movement, widget removal, and reset layout flows to save through the backend API, debouncing drag-position saves so intermediate pointer moves do not each issue a full `PUT`.
- [x] 3.4 Remove `localStorage` as the authoritative layout source while keeping only an optional one-time migration/cache fallback if needed.
- [x] 3.5 Keep widget layout shape compatible with existing grid coordinates (`x`, `y`, `w`, `h`) and fixed-size dashboard card behavior.

## 4. Mock Mode And UI Feedback

- [x] 4.1 Extend mock transport with `GET` and `PUT /api/dashboard/layout` using the same response envelope and revision behavior.
- [x] 4.2 Include deterministic mock dashboard layout data in `mockMode` reset.
- [x] 4.3 Surface layout load/save failures in the SPA without reverting to local storage as authoritative state.
- [x] 4.4 Preserve visual stability during backend layout load so dashboard widgets do not flash through transient positions.

## 5. Verification

- [x] 5.1 Add native firmware tests for layout validation, 8-panel limit, 32-character panel name limit, default generation, save/load, revision increments, and stale-device pruning.
- [x] 5.2 Add controller/API tests for successful `GET`, successful `PUT`, invalid JSON, unsupported schema, duplicate panel names, too many panels, overlong panel names, invalid active panel, and unsupported methods.
- [x] 5.3 Add frontend tests or smoke checks for backend layout load, debounced save after widget move, panel rename/delete, panel limit/name length validation, reset layout, and mock reset.
- [x] 5.4 Run `scripts/test.sh` for firmware verification.
- [x] 5.5 Run the portal SPA build/smoke checks and confirm bundle limits remain within spec.
