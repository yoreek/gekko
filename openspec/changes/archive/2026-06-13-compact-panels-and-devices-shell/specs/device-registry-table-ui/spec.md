## ADDED Requirements

### Requirement: Device registry is presented as a compact table
The SPA MUST provide a dedicated Devices page that presents the registry as a compact table instead of dashboard cards.

#### Scenario: Devices page renders a registry table
- **WHEN** the user opens the Devices route
- **THEN** the SPA shows a table of device records with compact row spacing

#### Scenario: Table shows core registry columns
- **WHEN** a device row is displayed
- **THEN** it shows the device ID, name, type, and `Effective_status`

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

### Requirement: Device registry rows can open device details
The SPA MUST let the user open a device detail dialog from the Devices table.

#### Scenario: Row opens details
- **WHEN** the user selects a device row or its detail action
- **THEN** the SPA opens the existing device detail dialog for that device

#### Scenario: Detail dialog stays the source of richer actions
- **WHEN** the device detail dialog opens from the Devices page
- **THEN** the user can still access the richer rename, enable, disable, delete, and command actions there
