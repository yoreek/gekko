## ADDED Requirements

### Requirement: Device detail modal provides contextual field hints
The SPA SHALL show short contextual hints for the switch detail fields that are most likely to be misunderstood.

#### Scenario: Switch guidance is visible
- **WHEN** the detail modal is open for a supported switch device
- **THEN** the SPA shows hints for `Safe state`, `Startup state`, and `Restore previous state`

#### Scenario: Hint text is localizable
- **WHEN** the modal renders a hinted field in English or Russian
- **THEN** the hint text uses the active locale messages instead of hard-coded copy

#### Scenario: Hint presentation stays compact
- **WHEN** a hint is rendered in the detail modal
- **THEN** it uses a compact Vuetify-native presentation such as helper text or tooltip content without expanding the dialog into a long help page

### Requirement: Device detail modal supports explicit edit mode
The SPA SHALL provide an explicit edit mode for supported device detail sections while keeping the default detail view readonly.

#### Scenario: Edit mode is entered explicitly
- **WHEN** the user activates the edit control in the detail modal
- **THEN** the modal switches from readonly view to editable controls for the fields that the device type exposes as editable

#### Scenario: Edit mode can be cancelled
- **WHEN** the user cancels edit mode
- **THEN** the modal returns to the readonly view without committing changes

#### Scenario: Edit mode preserves unsupported readonly fields
- **WHEN** a device type does not expose a field as editable
- **THEN** that field remains readonly in edit mode

#### Scenario: Edit mode saves through the existing device flow
- **WHEN** the user saves supported detail edits
- **THEN** the SPA submits the update through the existing device API contract and refreshes the dialog from the returned device snapshot

### Requirement: Device detail mutations keep the view synchronized
The SPA SHALL keep the open detail dialog and the cached device store synchronized after edit and delete actions by applying the returned snapshot and subsequent realtime device updates without a full registry reload.

#### Scenario: Edit refreshes the open dialog
- **WHEN** the user saves supported edits in the detail dialog
- **THEN** the dialog updates from the returned device snapshot and stays aligned with the shared device store

#### Scenario: Delete removes the device from the shared store
- **WHEN** a device is deleted from the list flow or from an equivalent supported action
- **THEN** the SPA removes the device from the shared device store and closes or clears the open detail view if it is showing that device

#### Scenario: Realtime updates keep the modal current
- **WHEN** a `device.upsert` or `device.remove` websocket message arrives after a mutation
- **THEN** the open detail view reflects the latest store state without forcing a full device registry reload

### Requirement: Type-specific detail panels can expose editable fields
The SPA SHALL let each supported device type provide its own editable detail controls through the existing device component registry.

#### Scenario: Switch type shows editable controls
- **WHEN** the selected device is a supported switch type
- **THEN** the type-specific detail panel can render editable controls for switch configuration fields

#### Scenario: Unsupported types remain readonly
- **WHEN** the selected device type has no edit form registered
- **THEN** the modal remains readonly for the type-specific section

#### Scenario: Future hints can be extended without redesign
- **WHEN** a later device type needs hints for additional fields such as `Inverted` or `GPIO pin`
- **THEN** the same hint mechanism can be reused without changing the dialog layout contract
