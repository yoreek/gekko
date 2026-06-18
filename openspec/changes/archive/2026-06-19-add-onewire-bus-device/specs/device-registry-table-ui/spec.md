## ADDED Requirements

### Requirement: Devices page supports OneWire bus forms
The SPA SHALL support creating, viewing, and editing OneWire bus devices through the shared Device form flow on the Devices page.

#### Scenario: OneWire create form shows pin config
- **WHEN** the user starts creating a `OneWireBusDevice`
- **THEN** the shared Device form shows the common name/enabled fields, a type-specific numeric GPIO pin input, and an internal pull-up toggle without board-specific pin restrictions

#### Scenario: OneWire edit form preserves type
- **WHEN** the user edits an existing OneWire bus device
- **THEN** the device type remains readonly and the user can update only supported mutable fields such as name, enabled state, GPIO pin, and internal pull-up

#### Scenario: OneWire form submits catalog type id
- **WHEN** the user submits a OneWire bus create request
- **THEN** the SPA sends `type_id = 3` with the type-specific config payload expected by the backend adapter

### Requirement: Devices page exposes OneWire scan results
The SPA SHALL let users start a OneWire scan and inspect discovered ROM addresses from the OneWire bus detail flow.

#### Scenario: User starts scan
- **WHEN** the user activates the OneWire scan control from the device detail view
- **THEN** the SPA posts the shared custom scan command to `POST /api/devices/:id/command`

#### Scenario: Scan progress is visible
- **WHEN** the selected OneWire bus snapshot reports `scan.in_progress = true`
- **THEN** the detail view shows a loading state and prevents duplicate scan submissions until the active scan completes or fails

#### Scenario: Scan results are listed
- **WHEN** the selected OneWire bus snapshot reports ready scan devices
- **THEN** the detail view lists each device's uppercase ROM address and family code so a future sensor create flow can reuse the selected address

#### Scenario: Empty scan result is clear
- **WHEN** the selected OneWire bus snapshot reports `scan.ready = true` and `scan.device_count = 0`
- **THEN** the detail view shows an empty-result state without presenting stale addresses
