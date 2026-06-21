## ADDED Requirements

### Requirement: Shared device detail dialog exposes recent device events
The SPA SHALL include a recent device event journal section in the shared device detail dialog used by device-aware entry points.

#### Scenario: Detail dialog shows collapsed recent events section
- **WHEN** the shared device detail dialog opens for a selected device in view mode
- **THEN** it shows a recent events section using standard Vuetify expansion panel behavior with the section collapsed by default

#### Scenario: Recent events are device-scoped
- **WHEN** the recent events section is expanded
- **THEN** it shows only retained journal entries whose device ID exactly matches the selected device

#### Scenario: Recent events are newest first and limited
- **WHEN** retained journal entries exist for the selected device
- **THEN** the section shows at most five entries ordered newest first

#### Scenario: Recent events do not add filters to detail dialog
- **WHEN** the recent events section is shown in the shared detail dialog
- **THEN** it does not render type, action, name, or ID filters

#### Scenario: Recent events update while dialog is open
- **WHEN** a supported device realtime message for the selected device arrives while the dialog is open
- **THEN** the recent events section updates from the journal store without requiring the dialog to be closed or refreshed
