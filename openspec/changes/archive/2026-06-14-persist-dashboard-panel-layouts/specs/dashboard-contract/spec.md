## MODIFIED Requirements

### Requirement: Dashboard widget layout
The SPA MUST allow compact device widgets to be reordered within the active panel in explicit edit mode, using a grid-based layout, and MUST persist the resulting layout through the backend dashboard layout API.

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
- The backend dashboard layout API MUST be the authoritative persistence source for saved widget coordinates.

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
- **WHEN** the user reloads the page after changing widget positions and the backend layout API is available
- **THEN** the SPA restores the saved widget layout returned by the backend

#### Scenario: Layout can be reset
- **WHEN** the user activates the dashboard reset layout control
- **THEN** the SPA rebuilds the active panel widget positions from the default grid placement and persists the reset layout through the backend layout API

#### Scenario: Read mode is not draggable
- **WHEN** edit mode is disabled
- **THEN** the widgets keep their saved positions and cannot be dragged
