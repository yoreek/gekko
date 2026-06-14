## Purpose

Define the dashboard contract for panel layout, widget layout, dashboard widget data, and route separation in the portal SPA.

## Requirements

### Requirement: Dashboard panel model
The SPA MUST present the dashboard as a panel-based surface with tabs, where only the active panel is shown.

#### Panel order
- The dashboard MUST allow panels to be reordered left-to-right.
- Panel reordering MUST preserve the active panel workflow and the saved panel layout.

#### Scenario: Active panel is visible
- **WHEN** the user opens the dashboard
- **THEN** the UI shows panel tabs and renders only the active panel content

#### Scenario: Only active panel content is mounted
- **WHEN** the dashboard renders panel content
- **THEN** it mounts the active panel content only and does not keep inactive panel grids mounted with active panel widgets

#### Scenario: Switching panels changes the visible content
- **WHEN** the user selects another panel tab
- **THEN** the dashboard switches to that panel without mixing in other panels

#### Scenario: Switching panels does not animate widget restoration
- **WHEN** the user switches dashboard tabs
- **THEN** the dashboard changes panel content without tab-window animation or visible widget position restoration

#### Scenario: Panels can be moved
- **WHEN** the user reorders panels
- **THEN** the dashboard moves the panels left or right in the tab order

### Requirement: Dashboard widget layout
The SPA MUST allow compact device widgets to be reordered within the active panel in explicit edit mode, using a grid-based layout, and MUST persist the resulting layout.

#### Grid coordinates
- Each widget MUST occupy grid coordinates, not free-floating pixels.
- Widget positions MUST be represented by grid coordinates such as `x`, `y`, `w`, and `h`.
- Dragging a widget MUST snap it to the grid coordinate system.
- Widgets MUST use fixed pixel-sized grid cells so card width does not stretch with viewport width.
- Dashboard device widgets MUST be fixed at `200px` wide and one compact row tall.
- Dashboard widget resizing MUST be disabled.
- The dashboard MUST recompute widget placement when the viewport size changes.
- If a widget no longer fits the available grid width after resize, the dashboard MUST move it to the nearest valid grid position without overlap.
- The dashboard MUST hide or defer widget grid presentation while restored coordinates are being applied, so users do not see intermediate positions.
- The dashboard MUST expose add-device behavior that filters out devices already present in the active panel.

#### Edit control
- The dashboard MUST expose an `Edit` button for the active panel.
- Pressing `Edit` MUST toggle widget edit mode for the current panel.

#### Scenario: Widgets can be dragged
- **WHEN** the user enables edit mode
- **THEN** the panel widgets become draggable and can be repositioned

#### Scenario: Widget can be removed in edit mode
- **WHEN** edit mode is enabled for the active panel
- **THEN** each widget shows a remove крестик and the user can delete the widget from the panel

#### Scenario: Device can appear in multiple panels
- **WHEN** the same device is added to another panel
- **THEN** the device remains in the original panel and is also shown in the target panel

#### Scenario: Duplicate widget in one panel is ignored
- **WHEN** the same device is added again to the active panel
- **THEN** the dashboard does not create a second widget entry for that device in that panel

#### Scenario: Grid movement is directional
- **WHEN** the user drags a widget in edit mode
- **THEN** the widget moves on the grid horizontally or vertically, including left, right, up, and down placement changes

#### Scenario: Layout is restored
- **WHEN** the user reloads the page after changing widget positions
- **THEN** the SPA restores the saved widget layout for the active panel

#### Scenario: Layout can be reset
- **WHEN** the user activates the dashboard reset layout control
- **THEN** the SPA rebuilds the active panel widget positions from the default grid placement and persists the reset layout

### Requirement: Dashboard layout reset does not preserve legacy storage
The dashboard layout contract SHALL treat the stored layout as current state only and SHALL reset to the default panel layout when the backend layout data is absent or invalid instead of migrating old JSON snapshots.

#### Scenario: Missing stored layout resets to default
- **WHEN** the backend has no valid stored dashboard layout
- **THEN** the dashboard shows the default panel layout rather than attempting to restore a legacy JSON snapshot

#### Scenario: Invalid stored layout resets to default
- **WHEN** the backend layout data is invalid or unreadable
- **THEN** the dashboard falls back to the default panel layout and does not preserve the old malformed data

#### Scenario: Add-device selector excludes current panel devices
- **WHEN** the user opens the add-device dialog for the active panel
- **THEN** the selector lists only devices that are not already present in that panel

#### Scenario: Read mode is not draggable
- **WHEN** edit mode is disabled
- **THEN** the widgets keep their saved positions and cannot be dragged

### Requirement: Dashboard widget data contract
The SPA MUST render each dashboard widget as a compact device card that contains only the device display name, with backend readiness expressed through visual state only.

#### Included data
- `name`
- optional compact device-type icon when it does not add height or metadata rows

#### Excluded data
- `device_id`
- `type` text
- `registry_revision`
- `config_revision`
- `pending_persistence`
- duplicated status chips
- extra metadata rows
- long-form device details
- `Ready` / `!Ready` text

#### Scenario: Widget content stays minimal
- **WHEN** a device widget is displayed
- **THEN** it shows the device name only

#### Scenario: Secondary state is visual
- **WHEN** a device widget is displayed
- **THEN** the backend status only affects the card's visual treatment

#### Scenario: Not ready widgets are dimmed
- **WHEN** a device widget has any backend status other than `ready`
- **THEN** the card is visually dimmed to indicate that it is not working

#### Scenario: Deleted devices are removed from layout
- **WHEN** a device is deleted from the registry
- **THEN** the dashboard removes that device from any panel layout that still references it

### Requirement: Dashboard panel availability
The SPA MUST keep at least one dashboard panel available at all times.

#### Scenario: Default panel exists
- **WHEN** the dashboard is opened for the first time or local layout data is reset
- **THEN** the SPA creates or restores a default panel

#### Scenario: Last panel cannot be removed
- **WHEN** the user tries to remove the last remaining panel
- **THEN** the SPA prevents the removal

### Requirement: Panel names are unique
The SPA MUST keep dashboard panel names unique.

#### Scenario: Duplicate panel names are rejected
- **WHEN** the user renames a panel to a name already used by another panel
- **THEN** the SPA rejects the duplicate name

### Requirement: Devices page is separate
The SPA MUST provide a dedicated Devices page for registry management instead of mixing registry controls into the dashboard.

#### Scenario: Devices is a separate view
- **WHEN** the user opens Devices
- **THEN** the SPA shows a dedicated registry management view

#### Scenario: Table supports compact management
- **WHEN** a device supports simple control actions
- **THEN** the Devices page exposes compact inline control where applicable

### Requirement: Portal routes stay separate
The SPA MUST keep WiFi, OTA, System, and Controller overview on dedicated routes.

#### Scenario: Dashboard route stays focused
- **WHEN** the user opens `/`
- **THEN** the SPA shows the dashboard only

#### Scenario: Overview uses the configured route
- **WHEN** the user opens Controller overview
- **THEN** the SPA loads `/overview`
