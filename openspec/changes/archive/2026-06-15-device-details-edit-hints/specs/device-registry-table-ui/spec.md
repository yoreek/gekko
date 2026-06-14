## ADDED Requirements

### Requirement: Devices table opens the shared editable detail dialog
The SPA SHALL open the same shared device detail dialog from the Devices table that is used elsewhere in the portal.

#### Scenario: Table row opens the shared dialog
- **WHEN** the user opens device details from the Devices table
- **THEN** the SPA opens the shared detail dialog for that device instead of a separate table-specific editor

#### Scenario: Detail behavior is consistent across entry points
- **WHEN** the detail dialog opens from the Devices page
- **THEN** it offers the same readonly view, edit mode, and field-hint behavior as the dashboard entry point

#### Scenario: Table entry points still support richer actions
- **WHEN** the detail dialog opens from the Devices page
- **THEN** the user can still access the existing rename, enable, disable, delete, and command actions from the same dialog

### Requirement: Devices table provides explicit row actions
The SPA SHALL expose an `Actions` column in the Devices table with direct `Edit` and `Delete` actions.

#### Scenario: Actions column is visible
- **WHEN** the Devices table renders
- **THEN** it includes an `Actions` column at the row level

#### Scenario: Edit action opens details
- **WHEN** the user activates `Edit` for a device row
- **THEN** the SPA opens the shared device detail dialog for that device

#### Scenario: Delete action removes from the table flow
- **WHEN** the user activates `Delete` for a device row
- **THEN** the SPA starts the delete flow from the table without requiring the detail dialog to be opened first

### Requirement: Devices table stays synchronized after edit and delete
The SPA SHALL update the Devices table from the returned mutation snapshot and subsequent realtime device messages after edit and delete actions without forcing a full registry reload.

#### Scenario: Edit updates the visible row
- **WHEN** the user saves edits for a device opened from the table
- **THEN** the table row refreshes from the returned snapshot and remains in sync with the shared device store

#### Scenario: Delete removes the row immediately
- **WHEN** the user deletes a device from the table
- **THEN** the table removes that row and keeps the registry cache aligned with the returned mutation state

#### Scenario: Realtime messages keep the table current
- **WHEN** a `device.upsert` or `device.remove` websocket message arrives after a mutation
- **THEN** the Devices table reflects the latest store state without requiring the user to refresh the page
