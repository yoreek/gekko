## MODIFIED Requirements

### Requirement: Device detail modal
The SPA SHALL open a modal dialog for a selected device and present a shared Device form surface with common fields first and type-specific details below, while making the current interaction mode visible in the dialog header.

#### Scenario: Device details open in modal
- **WHEN** a user selects a device widget or table row
- **THEN** the SPA opens a modal dialog containing the selected device details

#### Scenario: Modal shows shared base fields first
- **WHEN** the detail modal is open
- **THEN** it presents the shared device name, type, and enabled state before type-specific device details

#### Scenario: Mode is visible in the header
- **WHEN** the detail modal is open in View mode
- **THEN** the header shows a compact `View` indicator alongside the dialog title and status

#### Scenario: Edit mode is visible in the header
- **WHEN** the detail modal is open in Edit mode
- **THEN** the header shows a compact `Edit` indicator alongside the dialog title and status

#### Scenario: Modal delegates type-specific details
- **WHEN** the selected device has a registered type-specific detail section
- **THEN** the modal renders that section below the common fields through the device UI registry without an extra redundant wrapper title

#### Scenario: Modal stays synchronized
- **WHEN** the selected device changes due to a realtime update while the modal is open
- **THEN** the modal refreshes its visible state without closing

### Requirement: Device detail surface stays compact
The SPA SHALL keep the device detail modal visually compact and avoid redundant chrome around type-specific detail content.

#### Scenario: Redundant wrappers are omitted
- **WHEN** the modal renders type-specific details
- **THEN** it does not add a separate `Type-specific details` wrapper when the section title does not add useful information

#### Scenario: Compact chrome preserves readability
- **WHEN** the modal renders shared and type-specific fields
- **THEN** the layout keeps readable spacing and surface separation without adding unnecessary header rows or nested cards
