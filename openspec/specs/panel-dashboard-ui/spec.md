## Purpose

Define the panel-based dashboard UI, panel tabs, editable grid layout, and compact dashboard widgets in the portal SPA.

## Requirements

### Requirement: Panel dashboard uses tabs and active panel content
The SPA MUST present the dashboard as a panel-based view where the user selects the active panel from tabs and sees only the devices in that panel.

#### Scenario: Panels can be reordered
- **WHEN** the user drags a panel tab left or right
- **THEN** the dashboard reorders the panels in the tab strip

#### Scenario: Active panel is shown through tabs
- **WHEN** the dashboard opens
- **THEN** the UI shows panel tabs and renders the currently active panel content

#### Scenario: Only the active panel grid is mounted
- **WHEN** the dashboard renders the tab window
- **THEN** it mounts only the active panel's grid or empty state

#### Scenario: Active panel content is isolated
- **WHEN** a different panel tab is selected
- **THEN** the dashboard switches to that panel's devices without mixing the other panels into the content area

#### Scenario: Tab switching is visually stable
- **WHEN** the user switches between dashboard panels
- **THEN** the tab window changes content without animation and without showing transient widget coordinate restoration

### Requirement: Dashboard always has at least one panel
The SPA MUST ensure that a dashboard contains at least one panel at all times.

#### Scenario: Initial state creates a default panel
- **WHEN** the dashboard is opened for the first time or local panel data is reset
- **THEN** the SPA provides a default panel so the dashboard is never empty

#### Scenario: Last panel cannot be removed
- **WHEN** the user attempts to remove the last remaining panel
- **THEN** the SPA prevents the removal and keeps the default dashboard panel available

### Requirement: Panel names are unique
The SPA MUST keep dashboard panel names unique.

#### Scenario: Duplicate panel names are rejected
- **WHEN** the user renames a panel to a name already used by another panel
- **THEN** the SPA rejects the duplicate name

### Requirement: Dashboard widget layout is editable
The SPA MUST allow dashboard widgets to be reordered within the active panel in an explicit edit mode, using a grid-based layout, and must persist the resulting layout.

#### Grid coordinates
- Each widget MUST occupy grid coordinates instead of a free-floating position.
- Widget layout MUST be stored as coordinate data such as `x`, `y`, `w`, and `h`.
- Dragging a widget MUST snap it into the grid coordinate system.
- Widget grid cells MUST be fixed in pixels instead of stretching with the viewport.
- Dashboard device widgets MUST render as fixed-size `200px` wide, one-row cards.
- Dashboard widget resize controls MUST NOT be enabled.
- The dashboard MUST recompute widget placement when the viewport size changes.
- If a widget no longer fits the available grid width after resize, the dashboard MUST move it to the nearest valid grid position without overlap.
- Widget layout MUST remain compatible with backend persistence so the saved coordinates can later move to an API without changing the shape.

#### Scenario: Edit mode is toggled by button
- **WHEN** the user presses the `Edit` button on the active panel
- **THEN** the dashboard enters widget edit mode for that panel

#### Scenario: Widgets can be moved in edit mode
- **WHEN** the user enables dashboard edit mode
- **THEN** the panel widgets become draggable and can be repositioned within the active panel

#### Scenario: Widget can be removed in edit mode
- **WHEN** edit mode is enabled for the active panel
- **THEN** each widget shows a remove крестик and the user can delete the widget from the panel

#### Scenario: Devices may appear in multiple panels
- **WHEN** the user adds the same device to a different panel
- **THEN** the device remains present in the original panel and appears in the target panel as well

#### Scenario: Duplicate devices in one panel are ignored
- **WHEN** the user tries to add a device that is already present in the active panel
- **THEN** the dashboard does not create a duplicate widget and keeps the current panel layout stable

#### Scenario: Deleted devices are removed from layout
- **WHEN** a device is deleted from the registry
- **THEN** the dashboard removes that device from any panel layout that still references it

#### Scenario: Widgets move on grid axes
- **WHEN** the user drags a widget while edit mode is enabled
- **THEN** the widget can move left, right, up, or down on the grid and snap into a new grid position

#### Scenario: Layout changes persist
- **WHEN** the user changes widget positions in edit mode
- **THEN** the SPA stores the panel layout coordinates and restores them after refresh

#### Scenario: Layout can be reset
- **WHEN** the user presses the reset layout control
- **THEN** the active panel widgets return to their default grid coordinates and the reset positions are saved

#### Scenario: Add-device selector excludes current panel devices
- **WHEN** the user opens the add-device dialog for the active panel
- **THEN** the selector lists only devices that are not already present in that panel

#### Scenario: Read mode stays stable
- **WHEN** edit mode is disabled
- **THEN** the widgets remain in their saved positions and cannot be dragged

### Requirement: Dashboard widgets are compact and grid-based
The SPA MUST render dashboard device widgets as compact fixed-size cards that display only the device name, with backend readiness expressed through visual state only.

#### Scenario: Widget shows name only
- **WHEN** a device widget is displayed
- **THEN** the widget shows the device name only

#### Scenario: Not ready widgets are dimmed
- **WHEN** a device widget has any backend status other than `ready`
- **THEN** the card is visually dimmed to indicate that it is not working

#### Scenario: Widgets use grid layout
- **WHEN** the active panel contains widgets
- **THEN** the dashboard positions them on a grid that supports left, right, up, and down movement in edit mode
