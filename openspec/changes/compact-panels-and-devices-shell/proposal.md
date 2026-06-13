## Why

The current portal UI mixes dashboard cards, device management, and navigation in a layout that is too heavy for quick control work. We need a compact shell that matches the reference design more closely, keeps the dashboard focused on panels, and moves device registry management into a dedicated table view.

## What Changes

- Replace the current always-visible wide left drawer with a compact, button-opened sidebar shell similar to the reference project.
- Keep the top toolbar free of the selected menu item label.
- Add a top App bar menu button that opens the left navigation drawer on demand.
- Show the active panel name in the top toolbar when viewing the dashboard.
- Keep `Synced`, locale, theme, and mock indicators in the top toolbar.
- Add language switching and `light` / `dark` theme switching in the App bar.
- Keep all UI icons in a local registry inside the frontend codebase and do not add a separate icon package dependency.
- Keep local icons theme-aware by rendering SVG with `currentColor`.
- Add a dedicated `Panels` page that lists panels, allows renaming, and allows deletion.
- Keep panel names unique.
- Rework the main dashboard into a panel-based view with tabs at the top and only the active panel visible.
- Ensure at least one panel always exists and the dashboard never becomes empty.
- Render devices on the dashboard as compact fixed-size widgets arranged by saved panel layout coordinates.
- Keep dashboard widgets fixed at a compact `200px` width and disable widget resizing.
- Switch dashboard panels without tab-window animation or visible restored-position jumps.
- Support explicit dashboard edit mode so widgets can be dragged and reordered inside the active panel.
- Persist widget placement and restore it after refresh.
- Provide a reset layout control that restores the active panel to default widget coordinates.
- Keep dashboard widgets minimal: device name only, with `effective_status` used as visual-only dimming state.
- Dim dashboard widgets when backend status is not `ready`.
- Build dashboard widgets from a reusable base shell plus type-specific wrapper components so future device types can extend cleanly.
- Keep device UI code split into a dedicated components folder with a base device component and type-specific extensions.
- Add a dedicated `Devices` page for registry management with a compact table, `ID` filter, partial `Name` search, `Type` select filter, and inline control where applicable.
- Keep panel order and widget coordinates backend-friendly so a follow-up API can persist them without changing the data shape.
- Keep WiFi, OTA, System, and Controller overview as separate pages with their existing functional scope.
- Preserve the current local mock and realtime behavior while adapting the UI to the new shell and page split.

## Capabilities

### New Capabilities
- `panel-dashboard-ui`: panel-based dashboard behavior, panel selection, dashboard edit mode, and compact panel device presentation.
- `device-registry-table-ui`: dedicated device registry table view with search, type filtering, and detail access.

### Modified Capabilities
- `portal-web-app`: the SPA shell, sidebar navigation, toolbar composition, route entry points, and overall page structure change to match the reference-style layout.
- `device-dashboard-ui`: device presentation changes from tall registry cards to compact panel cards that show only `name` and dim when backend status is not `ready`.

## Impact

- Frontend Vue shell, routing, and page composition in `frontend/`.
- Device card and device detail components, plus a new table-based devices page.
- Local UI state for sidebar collapse and panel selection, persisted in browser storage.
- Smoke coverage and UI text/localization for the new navigation and page split.
- Production build size remains constrained; the primary gzipped JavaScript bundle target is under `200 kB`.
