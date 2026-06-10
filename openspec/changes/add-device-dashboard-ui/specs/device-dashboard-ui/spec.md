## ADDED Requirements

### Requirement: Device dashboard cards
The SPA SHALL present the device registry as a dashboard of device cards that summarize the common operational state of each device.

#### Scenario: Device cards are rendered
- **WHEN** the dashboard loads device registry data
- **THEN** it renders one card per device with the shared base fields visible at a glance

#### Scenario: Cards show common state
- **WHEN** a device card is displayed
- **THEN** it shows at least `device_id`, `type`, `name`, `status`, `enabled`, `registry_revision`, `config_revision`, and `pending_persistence`

#### Scenario: Cards reflect live updates
- **WHEN** a realtime device update arrives
- **THEN** the affected card updates without requiring a full page reload

### Requirement: Device detail modal
The SPA SHALL open a modal dialog for a selected device and present both shared fields and type-specific details.

#### Scenario: Device details open in modal
- **WHEN** a user selects a device card
- **THEN** the SPA opens a modal dialog containing the selected device details

#### Scenario: Modal shows shared base fields
- **WHEN** the detail modal is open
- **THEN** it presents the shared device base fields for every device type

#### Scenario: Modal stays synchronized
- **WHEN** the selected device changes due to a realtime update while the modal is open
- **THEN** the modal refreshes its visible state without closing

### Requirement: Device actions are available from the modal
The SPA SHALL allow the user to rename, enable, disable, delete, and command a device from the detail modal.

#### Scenario: Device can be renamed
- **WHEN** the user submits a new name from the modal
- **THEN** the SPA sends the rename through the existing device API path and updates the view on success

#### Scenario: Device can be enabled or disabled
- **WHEN** the user toggles the enabled state from the modal
- **THEN** the SPA sends the corresponding enable or disable action through the existing device API path and reflects the returned state

#### Scenario: Device can be deleted
- **WHEN** the user confirms a delete action from the modal
- **THEN** the SPA sends the delete request, closes or refreshes the modal as needed, and removes the device card when deletion succeeds

#### Scenario: Device command can be executed
- **WHEN** the user submits a command from the modal
- **THEN** the SPA sends the command to `POST /api/devices/:id/command` and shows the returned result or validation error

### Requirement: DummyDevice typed view is available
The SPA SHALL render a typed detail section for `DummyDevice` while keeping the dashboard extensible for future device types.

#### Scenario: DummyDevice gets a typed panel
- **WHEN** the selected device is a `DummyDevice`
- **THEN** the modal renders the typed `DummyDevice` section instead of only a raw JSON summary

#### Scenario: Unknown device type falls back safely
- **WHEN** the selected device type does not have a typed renderer yet
- **THEN** the modal still renders the shared base fields and a safe fallback view without breaking the dashboard

#### Scenario: Typed view preserves shared behavior
- **WHEN** a typed renderer is shown for a device
- **THEN** it still exposes the same shared base actions and status fields as the generic device view
