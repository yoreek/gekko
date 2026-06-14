## ADDED Requirements

### Requirement: Device update topics carry canonical device snapshots
The firmware SHALL publish realtime `device.upsert` and `device.command_result` messages with a canonical full device snapshot payload that the frontend can merge directly into its store.

#### Scenario: Device update includes full snapshot
- **WHEN** a device is created, updated, or its command result changes visible state
- **THEN** the websocket payload includes the full device snapshot fields used by REST responses, including the current runtime `output` when available

#### Scenario: Device update payload is mergeable
- **WHEN** the frontend receives a `device.upsert` or `device.command_result` message
- **THEN** it can update the device store from the payload alone without requesting a follow-up `/api/devices` refresh

#### Scenario: Device removal remains identity-only
- **WHEN** a device is deleted
- **THEN** the firmware continues to send a lightweight `device.remove` message that identifies the removed device by id
