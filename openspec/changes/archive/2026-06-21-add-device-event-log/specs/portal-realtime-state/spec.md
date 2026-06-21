## MODIFIED Requirements

### Requirement: Device update topics carry canonical device snapshots
The firmware SHALL publish realtime `device.upsert` and `device.command_result` messages with a canonical full device snapshot payload that the frontend can merge directly into its store when a snapshot is available.

#### Scenario: Device update includes full snapshot
- **WHEN** a device is created, updated, or its command result changes visible state
- **THEN** the websocket payload includes the full device snapshot fields used by REST responses, including the current runtime `output` when available

#### Scenario: Device update payload is mergeable
- **WHEN** the frontend receives a `device.upsert` or snapshot-bearing `device.command_result` message
- **THEN** it can update the device store from the payload alone without requesting a follow-up `/api/devices` refresh

#### Scenario: Device removal includes lightweight delete metadata
- **WHEN** a device is deleted
- **THEN** the firmware sends a lightweight `device.remove` message that identifies the removed device by id and includes `event_kind`, registry revision, pending persistence state, and last-known name and type metadata when available from a temporary pre-delete snapshot only

## ADDED Requirements

### Requirement: Device realtime payloads expose event kind
The firmware SHALL include explicit `event_kind` metadata in supported device realtime payloads without changing the top-level WebSocket envelope.

#### Scenario: Upsert event kind maps internal device events
- **WHEN** the firmware publishes a `device.upsert` message for an internal device event
- **THEN** the payload `event_kind` identifies the originating event as `device_created`, `device_updated`, `status_changed`, `state_changed`, `config_persisted`, or `retained_state_changed`

#### Scenario: Snapshot event kind marks startup state
- **WHEN** the firmware publishes a startup or reconnect device snapshot through `device.upsert`
- **THEN** the payload `event_kind` is `snapshot`

#### Scenario: Command result event kind marks command outcome
- **WHEN** the firmware publishes a `device.command_result` message
- **THEN** the payload `event_kind` is `command_accepted` or `command_rejected`

#### Scenario: Delete event kind marks removal
- **WHEN** the firmware publishes a `device.remove` message
- **THEN** the payload `event_kind` is `device_deleted`

### Requirement: Frontend realtime bridge records device journal entries
The SPA SHALL record local device event journal entries as part of handling supported device realtime topics.

#### Scenario: Device upsert records from event kind
- **WHEN** a `device.upsert` realtime message arrives
- **THEN** the frontend classifies the journal action from payload `event_kind` and records the journal entry before or alongside updating the device registry store

#### Scenario: Device command result records command event
- **WHEN** a `device.command_result` realtime message arrives
- **THEN** the frontend records a `command` journal entry using payload `event_kind` and merges the payload into the device registry store only when it contains a full device snapshot

#### Scenario: Device remove records before deletion
- **WHEN** a `device.remove` realtime message arrives
- **THEN** the frontend records a `deleted` journal entry using payload `event_kind` and delete metadata before removing the device from the device registry store

#### Scenario: Registry merge behavior is preserved
- **WHEN** a supported device realtime message is recorded in the journal
- **THEN** the SPA still updates the relevant Pinia device registry state according to the existing realtime merge requirements

#### Scenario: Unsupported realtime topics do not affect the journal
- **WHEN** the frontend receives an unsupported or non-device realtime topic
- **THEN** it does not append a device event journal entry and continues processing the message without corrupting store state
