## MODIFIED Requirements

### Requirement: Panel dashboard uses tabs and active panel content
The SPA MUST present the dashboard as a panel-based view where the user selects the active panel from tabs and sees only the devices in that panel, while providing explicit controls to move the active panel left or right.

#### Scenario: Panels can be reordered
- **WHEN** the user reorders a panel through the dashboard controls
- **THEN** the dashboard updates the panel order in the tab strip

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

#### Scenario: Active panel can be moved left
- **WHEN** the user presses the dashboard move-left control
- **THEN** the dashboard moves the active panel one position to the left and keeps the saved layout state

#### Scenario: Active panel can be moved right
- **WHEN** the user presses the dashboard move-right control
- **THEN** the dashboard moves the active panel one position to the right and keeps the saved layout state

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
