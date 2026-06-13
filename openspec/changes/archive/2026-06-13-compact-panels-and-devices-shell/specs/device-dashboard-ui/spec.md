## MODIFIED Requirements

### Requirement: Device dashboard cards
The SPA SHALL present the device dashboard as a panel widget surface of compact fixed-size device cards that show only the device display name, with readiness expressed through visual state only.

### Requirement: Device widgets are split by component responsibility
The SPA SHALL organize device widgets in a dedicated component area with a base device widget component and type-specific extensions.

#### Card composition
- The dashboard card MUST show only the device name.
- The dashboard card MAY include an optional compact device-type icon only if it does not increase the card height or add extra metadata rows.
- The dashboard card MUST NOT render `Ready` or `!Ready` text.
- The dashboard card MUST dim the card when the backend status is not `ready`.
- The dashboard card MUST use a fixed compact footprint and MUST NOT expose resize handles.
- The dashboard card MAY be implemented as a reusable base card shell with type-specific specialized components layered on top.

#### Scenario: Base widget is reused
- **WHEN** a device type does not need custom rendering
- **THEN** the SPA renders it with the base device widget component

#### Scenario: Type-specific widget can extend base
- **WHEN** a device type needs custom rendering in the future
- **THEN** the SPA can render it with a type-specific component layered on top of the base widget component

#### Scenario: Device widgets are rendered
- **WHEN** the dashboard loads the active panel layout
- **THEN** it renders one widget per device in the saved panel layout

#### Scenario: Widgets show primary state only
- **WHEN** a device widget is displayed
- **THEN** it shows the device name

#### Scenario: Not ready widgets are dimmed
- **WHEN** a device widget has any backend status other than `ready`
- **THEN** the card is visually dimmed to indicate that it is not working

#### Scenario: Widgets reflect live updates
- **WHEN** a realtime device update arrives
- **THEN** the affected widget updates without requiring a full page reload

### Requirement: Device detail modal
The SPA SHALL open a modal dialog for a selected device and present both shared fields and type-specific details.

#### Scenario: Device details open in modal
- **WHEN** a user selects a device widget or table row
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
- **THEN** the SPA sends the delete request, closes or refreshes the modal as needed, and removes the widget when deletion succeeds

#### Scenario: Device command can be executed
- **WHEN** the user submits a command from the modal
- **THEN** the SPA sends the command to `POST /api/devices/:id/command` and shows the returned result or validation error

### Requirement: DummyDevice typed view is available
The SPA SHALL render a typed detail section for `DummyDevice` while keeping the shared base fields visible in the modal.

#### Scenario: DummyDevice gets a typed panel
- **WHEN** the selected device is the supported `DummyDevice`
- **THEN** the modal renders the typed `DummyDevice` panel with the shared base fields and typed controls

#### Scenario: Shared base fields remain visible
- **WHEN** the detail modal is open for a supported device
- **THEN** it presents the shared device base fields alongside the typed `DummyDevice` section
