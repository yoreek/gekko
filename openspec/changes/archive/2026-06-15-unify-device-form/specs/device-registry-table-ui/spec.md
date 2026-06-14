## ADDED Requirements

### Requirement: Devices table opens the shared Device form
The SPA SHALL use the same shared Device form behavior when a device is opened from the Devices table as when it is opened from the dashboard.

#### Scenario: Table detail action opens shared form
- **WHEN** the user opens device details from the Devices table
- **THEN** the SPA opens the shared Device form in the detail modal with common fields first and type-specific details below

#### Scenario: Table edit action uses shared edit layout
- **WHEN** the user activates `Edit` for a device row
- **THEN** the SPA opens or switches to the shared Device form Edit mode instead of a table-specific editor

#### Scenario: Table create action uses shared create layout
- **WHEN** the user starts creating a device from the Devices page
- **THEN** the SPA shows the same shared Device form Create layout used by other create entry points
