## 1. Shell and Navigation

- [x] 1.1 Replace the permanent wide drawer with a compact sidebar opened from the top toolbar.
- [x] 1.2 Remove the active route label from the top toolbar and keep only shared status indicators there.
- [x] 1.3 Add top-toolbar controls for active panel name, `Synced`, locale, and mock state.

## 2. Dashboard Panels and Layout

- [x] 2.1 Introduce a frontend panel store that keeps at least one default panel and persists active panel selection.
- [x] 2.2 Model dashboard widgets with saved layout coordinates so panel layout survives refresh.
- [x] 2.3 Add dashboard edit mode with drag/reorder behavior for the active panel widgets.
- [x] 2.4 Render dashboard widgets as compact control-first cards with `effective_status` as a secondary indicator.
- [x] 2.5 Keep dashboard content limited to the active panel and its widgets without extra content blocks below.

## 3. Devices Registry Table

- [x] 3.1 Add a dedicated Devices route and page that renders the registry as a compact table.
- [x] 3.2 Add search-by-name and type-filter controls to the Devices page.
- [x] 3.3 Expose inline control actions in the table for managed device types and keep detail dialog access.
- [x] 3.4 Keep `effective_status` visible in the table while preserving primary control state presentation for managed devices.

## 4. Localization, Mock State, and Verification

- [x] 4.1 Update portal and dashboard localization strings for the new shell, dashboard, and devices page labels.
- [x] 4.2 Persist and reset the new panel layout storage together with mock reset handling.
- [x] 4.3 Update smoke coverage to verify the compact sidebar, dashboard panels, editable layout, and Devices table behavior.
