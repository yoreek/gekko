## ADDED Requirements

### Requirement: Device realtime messages expose journal metadata
The firmware SHALL include explicit journal metadata in supported device realtime payloads so the SPA can display message-level history without inferring event meaning from local state.

#### Scenario: Device lifecycle event includes event kind
- **WHEN** the firmware publishes a device realtime message caused by an internal device event
- **THEN** the payload includes an `event_kind` value that identifies the originating event type

#### Scenario: Startup snapshot includes snapshot event kind
- **WHEN** the firmware publishes startup or reconnect device snapshots
- **THEN** each snapshot payload includes `event_kind` equal to `snapshot`

#### Scenario: Command result includes command event kind
- **WHEN** the firmware publishes a `device.command_result` realtime message
- **THEN** the payload includes `event_kind` equal to `command_accepted` or `command_rejected`

#### Scenario: Delete event includes last-known metadata
- **WHEN** the firmware publishes a `device.remove` realtime message for a device whose runtime metadata was available before deletion
- **THEN** the payload includes `event_kind` equal to `device_deleted`, the removed device ID, last-known name, last-known type metadata, registry revision, and pending persistence state, with those metadata fields sourced from a temporary pre-delete snapshot only

### Requirement: Frontend records a bounded device event journal
The SPA SHALL maintain a bounded local device event journal for supported device realtime messages received during the current browser session.

#### Scenario: Created device event is recorded
- **WHEN** the SPA receives a device realtime message with `event_kind` equal to `device_created`
- **THEN** the SPA appends a journal entry with action `created`, the raw event kind, local receive time, device ID, name, device type, source topic, registry revision, and expandable message details

#### Scenario: Updated device event is recorded
- **WHEN** the SPA receives a device realtime message with `event_kind` equal to `device_updated`, `status_changed`, `state_changed`, `config_persisted`, or `retained_state_changed`
- **THEN** the SPA appends a journal entry with action `updated`, the raw event kind, local receive time, device ID, name, device type, source topic, registry revision, and expandable message details

#### Scenario: Deleted device event is recorded
- **WHEN** the SPA receives a `device.remove` realtime message
- **THEN** the SPA appends a journal entry with action `deleted`, the raw event kind, local receive time, removed device ID, source topic, registry revision, and name and device type metadata when available

#### Scenario: Command event is recorded
- **WHEN** the SPA receives a `device.command_result` realtime message with `event_kind` equal to `command_accepted` or `command_rejected`
- **THEN** the SPA appends a journal entry with action `command`, the raw event kind, local receive time, device ID when available, name and device type when available, source topic, registry revision, and expandable message details

#### Scenario: Snapshot event is recorded
- **WHEN** the SPA receives a device realtime message with `event_kind` equal to `snapshot`
- **THEN** the SPA appends a journal entry with action `snapshot`, the raw event kind, local receive time, device ID, name, device type, source topic, registry revision, and expandable message details

#### Scenario: Journal size remains bounded
- **WHEN** appending a journal entry would exceed the configured maximum journal size
- **THEN** the SPA keeps the newest entries and drops the oldest entries

#### Scenario: Journal is session-local
- **WHEN** the SPA is reloaded
- **THEN** previous journal entries are not restored from firmware storage, browser storage, or a REST API

### Requirement: Device event journal page presents realtime entries
The SPA SHALL provide a dedicated device event journal page that presents journal entries in reverse chronological order.

#### Scenario: Journal page shows core columns
- **WHEN** the user opens the device event journal page
- **THEN** the page shows entries with local receive time, device ID, name, device type, event kind, and action columns

#### Scenario: Newest events appear first
- **WHEN** multiple journal entries exist
- **THEN** the page shows the newest received entries before older entries

#### Scenario: Page updates dynamically
- **WHEN** a supported device realtime message arrives while the journal page is open
- **THEN** the new journal entry appears without a full page reload or manual refresh

#### Scenario: Event details expand from row control
- **WHEN** the user activates an event row details control
- **THEN** the page expands that entry to show the event topic, registry revision, event kind, action, local receive time, and message payload details

### Requirement: Device event journal page filters entries locally
The SPA SHALL filter device event journal entries locally without requiring a server-side search API.

#### Scenario: Type filter narrows events
- **WHEN** the user selects a device type filter
- **THEN** the page shows only journal entries for that device type

#### Scenario: Action filter narrows events
- **WHEN** the user selects an action filter
- **THEN** the page shows only journal entries with that action

#### Scenario: Name filter matches partially
- **WHEN** the user enters a partial device name
- **THEN** the page shows only journal entries whose device name contains the query case-insensitively

#### Scenario: ID filter matches exactly
- **WHEN** the user enters a device ID
- **THEN** the page shows only journal entries whose numeric device ID matches exactly

#### Scenario: Empty filters show all entries
- **WHEN** the type, action, name, and ID filters are unset
- **THEN** the page shows all retained journal entries

### Requirement: Device detail shows recent event journal
The SPA SHALL expose recent device event history from the shared device detail dialog without filters.

#### Scenario: Recent journal is collapsed by default
- **WHEN** a device detail dialog is opened in view mode
- **THEN** the dialog shows a recent event journal section that is collapsed by default

#### Scenario: Recent journal is limited to selected device
- **WHEN** the user expands the recent event journal section for a device
- **THEN** the section shows only entries whose device ID exactly matches that device

#### Scenario: Recent journal shows latest five events
- **WHEN** more than five retained journal entries exist for the selected device
- **THEN** the section shows only the five newest entries in reverse chronological order

#### Scenario: Recent journal updates dynamically
- **WHEN** a supported realtime message for the selected device arrives while the detail dialog is open
- **THEN** the recent event journal section reflects the new entry without reopening the dialog
