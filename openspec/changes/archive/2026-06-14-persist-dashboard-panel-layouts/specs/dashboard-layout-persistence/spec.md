## ADDED Requirements

### Requirement: Dashboard layout is persisted on the controller
The firmware SHALL persist the dashboard panel layout as a bounded versioned document containing panel order, active panel id, panel names, and device widget grid coordinates.

#### Scenario: Stored layout is restored
- **WHEN** a client requests the dashboard layout after a previous successful save
- **THEN** the firmware returns the saved panel order, active panel id, panel names, and widget coordinates

#### Scenario: Default layout is returned when storage is empty
- **WHEN** no dashboard layout has been saved on the controller
- **THEN** the firmware returns a deterministic default layout with at least one panel

#### Scenario: Layout document is versioned
- **WHEN** the firmware returns a dashboard layout
- **THEN** the response includes the stored layout schema version and a layout revision

### Requirement: Dashboard layout API is available
The firmware SHALL expose REST endpoints for loading and replacing the dashboard layout document.

#### Scenario: Layout can be loaded
- **WHEN** a client sends `GET /api/dashboard/layout`
- **THEN** the firmware returns a success envelope containing `revision` and `layout`

#### Scenario: Layout can be replaced
- **WHEN** a client sends a valid complete layout document to `PUT /api/dashboard/layout`
- **THEN** the firmware stores the normalized layout atomically and returns the saved layout with an incremented revision

#### Scenario: Unsupported methods are rejected
- **WHEN** a client sends an unsupported method to `/api/dashboard/layout`
- **THEN** the firmware returns the shared API error envelope instead of serving the SPA fallback

### Requirement: Dashboard layout validation is bounded
The firmware SHALL validate dashboard layout payloads before storing them and SHALL reject malformed, oversized, unsupported, or inconsistent layout documents.

#### Scenario: Malformed layout is rejected
- **WHEN** a client sends malformed JSON or a layout payload that exceeds the bounded request size
- **THEN** the firmware rejects the request with the shared API error envelope and does not replace the stored layout

#### Scenario: Unsupported schema is rejected
- **WHEN** a client sends a layout with an unsupported `schema_version`
- **THEN** the firmware rejects the request and keeps the previous layout

#### Scenario: At least one panel is required
- **WHEN** a client sends a layout with no panels
- **THEN** the firmware rejects the layout and keeps the previous layout

#### Scenario: Panel count is bounded
- **WHEN** a client sends a layout with more than 8 panels
- **THEN** the firmware rejects the layout and keeps the previous layout

#### Scenario: Panel names remain unique
- **WHEN** a client sends a layout with duplicate panel names
- **THEN** the firmware rejects the layout and keeps the previous layout

#### Scenario: Panel name length is bounded
- **WHEN** a client sends a panel name longer than 32 characters
- **THEN** the firmware rejects the layout and keeps the previous layout

#### Scenario: Active panel must exist
- **WHEN** a client sends a layout whose `active_panel_id` does not match an included panel
- **THEN** the firmware rejects the layout and keeps the previous layout

#### Scenario: Widget coordinates are validated
- **WHEN** a client sends a widget with invalid `device_id`, `x`, `y`, `w`, or `h` values
- **THEN** the firmware rejects the layout and keeps the previous layout

### Requirement: Deleted or unknown devices are pruned from persisted layout
The firmware SHALL prevent saved dashboard layouts from returning widget entries for devices that do not exist in the registry.

#### Scenario: Unknown widget device is not returned
- **WHEN** a saved layout contains a widget for a device id that is not in the registry
- **THEN** the firmware omits that widget from the returned layout

#### Scenario: Deleted device is removed from saved layout
- **WHEN** a device is deleted from the registry
- **THEN** subsequent dashboard layout loads do not include widgets for that deleted device

#### Scenario: Empty panel set recovers to default
- **WHEN** pruning invalid device widgets leaves the layout without a valid panel layout
- **THEN** the firmware returns a deterministic default layout with at least one panel
