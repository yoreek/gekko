## MODIFIED Requirements

### Requirement: Bus device detail views surface runtime diagnostics
The SPA SHALL render bus diagnostics in a dedicated runtime section for supported bus device detail views.

#### Scenario: I2C bus detail shows diagnostics
- **WHEN** the user opens an I2C bus device detail view
- **THEN** the UI shows the nested runtime diagnostics counters in a compact diagnostics section

#### Scenario: SPI bus detail shows diagnostics
- **WHEN** the user opens an SPI bus device detail view
- **THEN** the UI shows the nested runtime diagnostics counters in a compact diagnostics section

#### Scenario: Diagnostics stay separate from config
- **WHEN** the bus detail view renders diagnostics
- **THEN** it keeps the diagnostic counters separate from persisted configuration fields

### Requirement: I2C bus detail shows scan state
The SPA SHALL show the current I2C scan state and discovered addresses in the I2C bus detail view when a scan has been requested.

#### Scenario: Scan progress is visible
- **WHEN** an I2C bus scan is in progress
- **THEN** the detail view shows that the scan is active and can display the discovered addresses already collected

#### Scenario: Completed scan is visible
- **WHEN** an I2C bus scan completes
- **THEN** the detail view shows the completed result from runtime state without requiring a full page reload

### Requirement: Bus diagnostics can be reset from the detail view
The SPA SHALL provide a reset action in the supported bus device detail view so operators can clear transient diagnostics without changing configuration.

#### Scenario: Reset action is available
- **WHEN** the user opens a supported bus detail view
- **THEN** the UI exposes a diagnostics reset action for that bus runtime

#### Scenario: Reset action updates the view
- **WHEN** the user triggers diagnostics reset
- **THEN** the UI refreshes the visible diagnostics counters after the backend publishes the updated snapshot

