## MODIFIED Requirements

### Requirement: Device update topics carry canonical device snapshots
The firmware SHALL publish realtime `device.upsert` and `device.command_result` messages with a canonical nested device snapshot payload that the frontend can merge directly into its store when a snapshot is available.

#### Scenario: Device update includes full snapshot
- **WHEN** a device is created, updated, or its command result changes visible state
- **THEN** the websocket payload includes the full nested device snapshot fields used by REST responses, including the current runtime `output` when available

#### Scenario: Device update payload is mergeable
- **WHEN** the frontend receives a `device.upsert` or snapshot-bearing `device.command_result` message
- **THEN** it can update the device store from the payload alone without requesting a follow-up `/api/devices` refresh

#### Scenario: Device removal remains identity-only
- **WHEN** a device is deleted
- **THEN** the firmware sends a `device.remove` message that identifies the removed device by id without requiring a separate device record model

### Requirement: Realtime snapshots expose deps
The firmware SHALL publish canonical realtime device snapshots using `config.deps` and computed `has_deps`.

#### Scenario: Device update includes deps
- **WHEN** a device realtime snapshot is published
- **THEN** the payload includes the same `config.deps` and computed `has_deps` fields used by REST snapshots

#### Scenario: Legacy relationship fields are absent from realtime
- **WHEN** a device realtime snapshot is published after the dependency migration
- **THEN** the payload does not include legacy relationship fields

#### Scenario: Frontend merges deps
- **WHEN** the frontend receives a realtime device snapshot
- **THEN** it updates the device store dependency fields from the payload alone

#### Scenario: Realtime metadata stays outside device records
- **WHEN** a realtime message includes event metadata
- **THEN** event kind and message revision remain websocket metadata
- **AND** `pendingPersistence` is not copied into the device record
