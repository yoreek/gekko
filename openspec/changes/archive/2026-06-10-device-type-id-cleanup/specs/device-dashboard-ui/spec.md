## MODIFIED Requirements

### Requirement: DummyDevice typed view is available
The SPA SHALL render a typed detail section for `DummyDevice` while keeping the shared base fields visible in the modal.

#### Scenario: DummyDevice gets a typed panel
- **WHEN** the selected device is the supported `DummyDevice`
- **THEN** the modal renders the typed `DummyDevice` panel with the shared base fields and typed controls

#### Scenario: Shared base fields remain visible
- **WHEN** the detail modal is open for a supported device
- **THEN** it presents the shared device base fields alongside the typed `DummyDevice` section
