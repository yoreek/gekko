## Purpose

Define the Devices page registry table, frontend filters, inline controls, and detail-dialog entry points in the portal SPA.

## Requirements

### Requirement: Device registry is presented as a compact table
The SPA MUST provide a dedicated Devices page that presents the registry as a compact table instead of dashboard cards.

#### Scenario: Devices page renders a registry table
- **WHEN** the user opens the Devices route
- **THEN** the SPA shows a table of device records with compact row spacing

#### Scenario: Table shows core registry columns
- **WHEN** a device row is displayed
- **THEN** it shows the device ID, name, type, `Effective_status`, and an `Actions` column

#### Scenario: Table uses ready-made Vuetify components
- **WHEN** the Devices page renders the registry table
- **THEN** the SPA uses Vuetify table and form components instead of custom table primitives where Vuetify already covers the need

### Requirement: Device registry supports search and type filtering
The SPA MUST allow the user to search devices by name and filter the registry by device type.

#### Scenario: ID filter is available
- **WHEN** the Devices page is displayed
- **THEN** the user can filter the table by device ID

#### Scenario: ID filter is exact
- **WHEN** the user enters a device ID
- **THEN** the table shows only the device whose numeric ID matches exactly

#### Scenario: Search filters by partial name
- **WHEN** the user enters a partial device name in the search field
- **THEN** the table shows only devices whose names match the query

#### Scenario: Name search ignores case
- **WHEN** the user searches by name using any mix of uppercase and lowercase letters
- **THEN** the table matches device names case-insensitively

#### Scenario: Type filter narrows the table
- **WHEN** the user selects a device type filter
- **THEN** the table shows only devices of that type

#### Scenario: Filtering happens on the frontend
- **WHEN** the Devices page loads the registry
- **THEN** the page filters the full device list locally without requiring a dedicated server-side search API

### Requirement: Device registry rows support inline control where applicable
The SPA MUST expose inline control actions in the device table for device types that support management actions.

#### Scenario: Managed device shows an inline control
- **WHEN** a device supports direct control
- **THEN** its table row shows a compact control element that can change the device state

#### Scenario: Status-only device does not force a control
- **WHEN** a device does not support direct control
- **THEN** its table row may omit the control element and present its status information only

### Requirement: Devices table exposes registry deletion in an Actions column
The SPA MUST expose device deletion from the Devices page table `Actions` column instead of the shared detail dialog.

#### Scenario: Device row shows delete action
- **WHEN** a device row is displayed on the Devices page
- **THEN** the row `Actions` column contains a delete control for that device

#### Scenario: Delete action does not open details
- **WHEN** the user activates the delete control from the `Actions` column
- **THEN** the SPA opens delete confirmation for that device without opening or switching the detail dialog

#### Scenario: Device deletion updates registry views
- **WHEN** the user confirms deletion from the Devices page
- **THEN** the SPA sends the delete request, removes the device from the table, and removes any dashboard widget for that deleted device

### Requirement: Device registry rows can open device details
The SPA MUST let the user open a device detail dialog from the Devices table.

#### Scenario: Row opens details
- **WHEN** the user selects a device row or its detail action
- **THEN** the SPA opens the existing device detail dialog for that device

#### Scenario: Detail dialog stays the source of non-destructive details
- **WHEN** the device detail dialog opens from the Devices page
- **THEN** the user can still access the shared detail form, edit flow, and supported runtime commands there, but not the delete action

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
