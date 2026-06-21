## Purpose

Define the device event journal retained in the portal SPA and the event-kind semantics it uses for local filtering and detail inspection.

## Requirements

### Requirement: Device event journal retains supported realtime events
The SPA SHALL keep a bounded, session-local journal of supported device realtime events received through the portal WebSocket.

#### Scenario: Journal entry records event metadata
- **WHEN** the SPA receives a supported device realtime message
- **THEN** it appends a journal entry with the local receive time, raw `event_kind`, device ID, name, device type, source topic, registry revision, and expandable message details

#### Scenario: Journal keeps newest entries first
- **WHEN** new supported device realtime messages arrive
- **THEN** the SPA places the newest journal entries before older retained entries

#### Scenario: Journal remains bounded
- **WHEN** appending a new journal entry would exceed the configured journal size
- **THEN** the SPA keeps the newest retained entries and drops the oldest retained entries

#### Scenario: Journal is session-local
- **WHEN** the SPA reloads
- **THEN** it does not restore journal entries from firmware storage, browser storage, or a REST API

### Requirement: Device event journal page presents local event history
The SPA SHALL provide a dedicated device event journal page that presents journal entries in reverse chronological order.

#### Scenario: Journal page shows core columns
- **WHEN** the user opens the device event journal page
- **THEN** the page shows entries with local receive time, device ID, name, device type, and event kind columns

#### Scenario: Event kind is localized
- **WHEN** the journal page renders a supported device event kind
- **THEN** it shows a localized label for that event kind rather than the raw machine token

#### Scenario: Newest events appear first
- **WHEN** multiple journal entries exist
- **THEN** the page shows the newest received entries before older entries

#### Scenario: Page updates dynamically
- **WHEN** a supported device realtime message arrives while the journal page is open
- **THEN** the new journal entry appears without a full page reload or manual refresh

#### Scenario: Event details expand from row control
- **WHEN** the user activates an event row details control
- **THEN** the page expands that entry to show the event topic, registry revision, event kind, local receive time, and message payload details

### Requirement: Device event journal page filters entries locally
The SPA SHALL filter device event journal entries locally without requiring a server-side search API.

#### Scenario: Type filter narrows events
- **WHEN** the user selects a device type filter
- **THEN** the page shows only journal entries for that device type

#### Scenario: Event kind filter narrows events
- **WHEN** the user selects an event kind filter
- **THEN** the page shows only journal entries whose raw `event_kind` matches that filter

#### Scenario: Name filter matches partially
- **WHEN** the user enters a partial device name
- **THEN** the page shows only journal entries whose device name contains the query case-insensitively

#### Scenario: ID filter matches exactly
- **WHEN** the user enters a device ID
- **THEN** the page shows only journal entries whose numeric device ID matches exactly

#### Scenario: Empty filters show all entries
- **WHEN** the type, event kind, name, and ID filters are unset
- **THEN** the page shows all retained journal entries

#### Scenario: Empty states explain retained and filtered views
- **WHEN** there are no retained journal entries or no entries matching the current filters
- **THEN** the page shows an appropriate empty state for the current condition
