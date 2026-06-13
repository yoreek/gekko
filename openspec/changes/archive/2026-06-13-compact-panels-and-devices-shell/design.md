## Context

The current Vue portal has already split controller functions into separate routes, but the dashboard still behaves like a device list with oversized cards. The product direction now matches the `easy-iot-manager` reference more closely: a compact shell, dashboard panels as the primary landing surface, a separate devices registry table, and inline controls only where a device actually supports them.

The reference project uses a dashboard panel model with editable widget layout. Widgets have a stable place inside a panel and can be reordered in an edit mode. The current portal does not have a panel layout model yet, so this design introduces one in the frontend only, without changing the backend registry contract.

Constraints:
- Keep the existing firmware API contract intact.
- Keep mock mode and realtime update behavior working.
- Preserve `WiFi`, `OTA`, `System`, and `Controller overview` as separate pages.
- Avoid adding a second source of truth for device state.

## Goals / Non-Goals

**Goals:**
- Make the shell visually closer to the reference project.
- Replace the permanent wide sidebar with a compact, button-opened navigation shell.
- Put panel tabs at the top of the dashboard and show only the active panel content.
- Support dashboard edit mode so device widgets can be dragged and reordered within a panel.
- Keep device widgets compact and one-line oriented.
- Add a dedicated `Devices` page for table-based registry management.
- Keep dashboard widget status as visual-only dimming, not as visible text.
- Allow managed devices to surface their primary control state directly in the widget or table row.

**Non-Goals:**
- Do not change the backend device registry schema just to support the UI refactor.
- Do not introduce a new backend panel API in this change.
- Do not implement new device types beyond what the current firmware/frontend already supports.
- Do not redesign `WiFi`, `OTA`, `System`, or `Controller overview` beyond shell integration.

## Decisions

### 1. Use a compact sidebar that opens from the top toolbar
The shell should follow the reference pattern: the top bar owns the menu trigger, and the left navigation is not a permanently dominant content column.

Why:
- It frees horizontal space for dashboard panels and table content.
- It matches the interaction model already demonstrated in the reference app.
- It keeps the selected menu label out of the top toolbar, which reduces visual noise.

Alternatives considered:
- Permanent wide drawer: rejected because it consumes too much width on the dashboard.
- Hidden mobile-only drawer: rejected because we also want the same interaction on desktop.

### 2. Model dashboard content as panels with explicit widget layout
Dashboard devices should not simply flow by CSS wrap. Each panel needs a saved layout so widgets can be reordered and kept in place across refreshes.

Why:
- The reference app uses explicit positions and edit mode.
- A pure wrap layout cannot preserve intentional dashboard arrangements.
- The user wants drag/reorder, which implies layout state rather than only visual order.

Design choice:
- The frontend stores panel data locally for now, but the panel order and widget coordinate shape stay backend-compatible for a later persistence API.
- A panel contains an ordered widget layout with stable device references and layout metadata.
- Edit mode toggles drag/reorder behavior.
- Widget resize is not part of the dashboard behavior; widgets keep a fixed compact footprint.
- The dashboard uses fixed pixel grid cells so card width remains stable while the number of columns changes with available space.
- The active panel grid is the only grid mounted in the tab window, and widget presentation is deferred until restored coordinates have settled to avoid visible position jumps.
- A reset layout action restores the active panel widgets to their default grid coordinates.
- The default panel must always exist so the dashboard never becomes empty.
- The dashboard remains panel-based, with tabs selecting the active panel and only that panel rendered in the content area.
- Device widgets are compact and control-first; they are not meant to expand into tall registry cards.
- `effective_status` is used as a secondary visual state, not as the main dashboard label.
- Panel names must be unique so panel selection and persistence remain deterministic.

Alternatives considered:
- Auto-flow CSS grid only: rejected because it cannot preserve widget placement.
- Backend-managed panel layouts: deferred because this change is scoped to the UI refactor and the backend does not yet expose panel APIs.

### 3. Split dashboard and devices responsibilities
`Dashboard` becomes a panel-focused control surface. `Devices` becomes the registry management page.

Why:
- The dashboard needs to stay compact and fast to read.
- The device registry needs search, filtering, and row-level actions.
- Combining both concerns in one view creates the visual clutter that prompted this change.

Alternatives considered:
- Keep device registry cards on the dashboard: rejected because it conflicts with the panel-first layout.
- Move device management into modal-only interactions: rejected because the user explicitly wants table management and inline control where appropriate.

### 4. Treat dashboard device status as visual-only
The dashboard card should show only the device name. Backend `ready` status affects styling, not visible text.

Why:
- The dashboard must stay compact.
- The operational state should be recognizable from the card treatment without adding another line of text.
- The registry table and detail dialog already expose the explicit status text where needed.

Alternatives considered:
- Make the dashboard card show `Ready` / `!Ready` text: rejected because it adds noise.
- Hide readiness entirely: rejected because the card should still visually communicate non-ready devices.

### 5. Keep inline control on both dashboard widgets and device table rows where applicable
The table page is not read-only. If a device supports a simple control action, the table should expose it compactly.

Why:
- The user asked for management actions in the table as well.
- This keeps common actions close to the registry list.
- It reduces the need to open details for routine control operations.

Alternatives considered:
- Control only in the detail dialog: rejected because it adds friction for simple toggles.
- Control only on dashboard widgets: rejected because the table is the registry management surface.

### 6. Persist layout and shell preferences in browser storage
Sidebar collapse, active panel, and panel layout should persist locally.

Why:
- The current app already uses localStorage for mock and locale state.
- The dashboard layout is a user preference and should survive refresh.
- This can be done without backend migration work.

Alternatives considered:
- In-memory only: rejected because it loses user layout on refresh.
- Backend persistence: deferred until the layout model is mature.

### 7. Keep icons local and theme-aware
The SPA should use a small local SVG registry instead of adding an icon package dependency.

Why:
- The deployment bundle must stay size constrained.
- The portal runs offline from the controller.
- Local SVG paths can use `currentColor`, so toolbar and drawer icons follow the active theme without additional assets.

Alternatives considered:
- Vuetify default icon packages: rejected because they add dependency and bundle cost.
- External SVG assets or CDN icons: rejected because the SPA must run offline.

### 8. Prefer Vuetify and evaluate libraries for complex behavior
The SPA should use Vuetify components, props, defaults, and theme tokens before adding custom markup or CSS. Custom CSS is acceptable only when Vuetify does not provide the needed behavior or layout, and colors should come from Vuetify theme variables such as `surface`, `background`, and `on-surface`.

Why:
- Vuetify components already solve common UI behavior and accessibility details.
- Theme tokens prevent light/dark regressions from hard-coded colors.
- Component props/defaults are easier to maintain than CSS overrides against generated Vuetify internals.

For complex frontend interactions, implementation should evaluate existing libraries before writing custom behavior. Examples include collision-aware dashboard grids, virtualized lists, advanced tables, charts, and rich drag/drop. When a library has bundle-size or architecture trade-offs, options should be presented for user approval before adding the dependency.

Alternatives considered:
- Custom implementation first: rejected because it increases maintenance risk for interaction-heavy UI.
- Adding libraries without review: rejected because the controller bundle has size constraints and must remain offline-friendly.

## Risks / Trade-offs

- [Risk] Frontend-only panel persistence can diverge from a future backend panel model. → Mitigation: keep the stored shape simple and device-reference based so it can be mapped later.
- [Risk] Drag/reorder UI can become noisy if edit mode is not clearly separated. → Mitigation: make drag affordances visible only while editing.
- [Risk] Compact widgets may not fit future complex device types. → Mitigation: reserve detail dialog and table view for richer controls.
- [Risk] Managing state in both dashboard widgets and table rows can duplicate interaction logic. → Mitigation: keep the underlying command path shared and only vary presentation.
- [Risk] A later backend persistence API can drift from the local layout shape. → Mitigation: keep panel order and widget coordinates serialized as plain `x`, `y`, `w`, `h` metadata from the start.
- [Risk] Restored grid coordinates can visibly jump while a layout library initializes. → Mitigation: mount only the active panel grid and defer grid visibility until coordinate restoration has settled.
- [Risk] Hard-coded colors can break one of the themes. → Mitigation: use Vuetify theme tokens for surfaces and text by default.
- [Risk] Custom implementations of complex interactions can become brittle. → Mitigation: evaluate proven libraries and confirm trade-offs before adding dependencies.

## Migration Plan

1. Introduce the new shell and sidebar trigger without changing backend routes.
2. Add frontend panel state storage with a guaranteed default panel.
3. Move dashboard rendering to panel tabs plus layout-driven widgets.
4. Add the `Devices` page and keep detail modal actions intact.
5. Update smoke coverage to verify layout, navigation, and inline control.
6. Remove the old dashboard-only card assumptions once the new views are stable.

Rollback:
- If the new panel layout causes instability, fall back to a single default panel and keep the `Devices` table and sidebar shell.
- If drag/reorder proves too heavy for the current release, keep the dashboard read-only while preserving the new shell and page split.

## Open Questions

- Should the `Devices` table show per-device inline control only for managed types, or also expose sensor-oriented quick actions once those types exist?
- Should `Synced` in the toolbar represent registry sync only, or also reflect active panel layout sync state?
