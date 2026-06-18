## ADDED Requirements

### Requirement: OneWire bus detail components use the device UI registry
The SPA SHALL render OneWire bus widgets and detail content through the existing device component registry while preserving the dashboard's compact card behavior.

#### Scenario: Dashboard renders compact OneWire widget
- **WHEN** a OneWire bus device is included in the active dashboard panel layout
- **THEN** the dashboard renders it through the registered OneWire component while keeping the compact card rule that shows only the device name and readiness visual state

#### Scenario: Detail modal renders OneWire fields
- **WHEN** the user opens a OneWire bus device detail modal
- **THEN** the modal shows common device fields first and the type-specific bus pin, internal pull-up state, and scan state below them

#### Scenario: Realtime update refreshes scan state
- **WHEN** a realtime `StateChanged` device update changes OneWire scan progress or scan results while the detail modal is open
- **THEN** the modal refreshes the visible scan state without closing or requiring a full page reload

#### Scenario: Scan control is not shown for unsupported devices
- **WHEN** the selected device type is not `OneWireBusDevice`
- **THEN** the OneWire scan command control is not rendered in that device's detail surface
