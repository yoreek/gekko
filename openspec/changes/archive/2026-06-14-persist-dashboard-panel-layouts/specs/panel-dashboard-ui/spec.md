## MODIFIED Requirements

### Requirement: Dashboard widget layout is editable
The SPA MUST allow dashboard widgets to be reordered within the active panel in an explicit edit mode, using a grid-based layout, and must persist the resulting layout through the backend dashboard layout API.

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
- The backend dashboard layout API MUST be the authoritative persistence source for saved widget coordinates.

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
- **WHEN** the user changes widget positions in edit mode and the backend layout API is available
- **THEN** the SPA stores the panel layout coordinates through the backend and restores them after refresh

#### Scenario: Drag saves are debounced
- **WHEN** the user drags a widget across multiple intermediate grid positions
- **THEN** the SPA avoids sending a full layout save for every intermediate pointer movement and persists the final stable position through the backend layout API

#### Scenario: Layout can be reset
- **WHEN** the user presses the reset layout control
- **THEN** the active panel widgets return to their default grid coordinates and the reset positions are saved through the backend layout API

#### Scenario: Read mode stays stable
- **WHEN** edit mode is disabled
- **THEN** the widgets remain in their saved positions and cannot be dragged

## ADDED Requirements

### Requirement: Panel state loads from backend layout
The SPA MUST load panel order, panel names, active panel id, and widget coordinates from the backend dashboard layout API when the dashboard shell initializes.

#### Scenario: Backend layout initializes panels
- **WHEN** the dashboard loads and `GET /api/dashboard/layout` succeeds
- **THEN** the SPA renders panel tabs and active panel content from the returned backend layout

#### Scenario: Backend layout becomes authoritative
- **WHEN** the SPA successfully loads a backend layout
- **THEN** local browser storage is not treated as the authoritative panel layout source

#### Scenario: Missing backend layout uses default
- **WHEN** the backend returns a default layout because no saved layout exists
- **THEN** the SPA renders that default layout and can persist subsequent user changes through the backend

#### Scenario: Panel creation respects backend limit
- **WHEN** the dashboard already has 8 panels
- **THEN** the SPA prevents creating another panel and does not send an invalid layout save

#### Scenario: Panel name length respects backend limit
- **WHEN** the user enters a panel name longer than 32 characters
- **THEN** the SPA prevents saving the invalid name through the backend layout API

#### Scenario: Add-device selector excludes current panel devices
- **WHEN** the user opens the add-device dialog for the active panel
- **THEN** the selector lists only devices that are not already present in that panel
