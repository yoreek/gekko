## Purpose

Define the realtime `/ws` portal contract used by the frontend and firmware.

## Requirements

### Requirement: Single portal WebSocket endpoint
The firmware SHALL expose a single `/ws` WebSocket endpoint for portal realtime state updates.

#### Scenario: Client connects to WebSocket
- **WHEN** a frontend client opens `/ws`
- **THEN** the firmware accepts the connection and sends a compact initial message describing the connection and current revision context

#### Scenario: WebSocket coexists with HTTP portal
- **WHEN** the portal HTTP server is running in station mode or setup AP mode
- **THEN** the `/ws` endpoint is available from the same host and port as the SPA and REST API

#### Scenario: Client disconnects
- **WHEN** a WebSocket client disconnects
- **THEN** the firmware releases client state without affecting HTTP portal routes or other connected clients

### Requirement: Incremental state push messages
The firmware SHALL push small topic-based state messages instead of broadcasting one large all-state JSON object for every change.

#### Scenario: Realtime envelope is consistent
- **WHEN** the firmware sends a realtime state message
- **THEN** the JSON message uses `topic`, `revision`, and `payload` fields, where `payload` contains only the data needed for that topic

#### Scenario: Device record changes
- **WHEN** a device is created, updated, deleted, or receives a command result that changes visible state
- **THEN** the firmware broadcasts a small device topic message containing the affected device identity, relevant payload, and registry revision

#### Scenario: WiFi status changes
- **WHEN** the WiFi runtime status visible to the portal changes
- **THEN** the firmware broadcasts a small WiFi status topic message

#### Scenario: OTA status changes
- **WHEN** OTA status visible to the portal changes
- **THEN** the firmware broadcasts a small OTA status topic message

#### Scenario: No subscribers are connected
- **WHEN** a realtime state event occurs and no WebSocket clients are connected
- **THEN** the firmware drops the event without buffering an unbounded backlog

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

### Requirement: Realtime snapshots expose deps
The firmware SHALL publish canonical realtime device snapshots using `deps` and computed `has_deps`.

#### Scenario: Device update includes deps
- **WHEN** a device realtime snapshot is published
- **THEN** the payload includes the same `deps` and computed `has_deps` fields used by REST snapshots

#### Scenario: Legacy relationship fields are absent from realtime
- **WHEN** a device realtime snapshot is published after the dependency migration
- **THEN** the payload does not include legacy relationship fields

#### Scenario: Frontend merges deps
- **WHEN** the frontend receives a realtime device snapshot
- **THEN** it updates the device store dependency fields from the payload alone

### Requirement: Frontend realtime store
The SPA SHALL consume WebSocket messages through Pinia-backed state stores and recover from reconnects.

#### Scenario: WebSocket message updates store
- **WHEN** the frontend receives a supported realtime topic message
- **THEN** it updates the relevant Pinia store without requiring a full page reload

#### Scenario: WebSocket reconnects
- **WHEN** the WebSocket connection closes unexpectedly
- **THEN** the frontend attempts reconnect with bounded delay and refreshes REST snapshots only when revision metadata indicates missed or inconsistent state

#### Scenario: Unsupported message is ignored
- **WHEN** the frontend receives an unknown realtime topic
- **THEN** it ignores the message without breaking the connection or corrupting existing store state

### Requirement: DS18B20 realtime output updates
The firmware SHALL publish DS18B20 temperature output and lifecycle changes through the existing canonical device realtime topics.

#### Scenario: Changed temperature publishes device snapshot
- **WHEN** a DS18B20 runtime completes a successful poll and the temperature changed by at least its configured report delta according to its report policy
- **THEN** the firmware publishes a `device.upsert` or `device.command_result` payload containing the canonical DS18B20 snapshot with current `output.temperature`

#### Scenario: Report always publishes repeated readings
- **WHEN** a DS18B20 runtime has report-always enabled and a poll succeeds with an unchanged temperature
- **THEN** the firmware still publishes a canonical device snapshot for that completed reading

#### Scenario: Quiet unchanged reading does not publish
- **WHEN** a DS18B20 runtime has report-always disabled and a poll succeeds with a temperature change below its configured report delta
- **THEN** the firmware does not emit a realtime device update solely for that poll

#### Scenario: Missing reading publishes invalid output when state changes
- **WHEN** a DS18B20 runtime transitions from valid output to unavailable output after startup, reconfiguration, dependency blocking, or read failure
- **THEN** the firmware publishes a canonical device snapshot with `output.temperature.valid = false`

#### Scenario: Dependency status changes are reflected
- **WHEN** a DS18B20 dependency bus is disabled, reconfigured, faulted, or returns ready
- **THEN** realtime device snapshots for affected DS18B20 dependents reflect the updated lifecycle or effective status without requiring a full page reload

#### Scenario: Frontend store merges DS18B20 updates
- **WHEN** the SPA receives a realtime DS18B20 device snapshot
- **THEN** it updates the device store temperature output, unit, status, dependency fields, and config revision from the payload alone

### Requirement: Thermostat realtime updates
The firmware SHALL publish thermostat runtime output, dep status, and lifecycle changes through the existing canonical device realtime topics.

#### Scenario: Control change publishes snapshot
- **WHEN** a thermostat runtime changes desired switch output, output status, latest valid temperature state, or the downstream switch reports a new actual output state
- **THEN** the firmware publishes a `device.upsert` or `device.command_result` payload containing the canonical thermostat snapshot

#### Scenario: Dep status change publishes thermostat snapshot
- **WHEN** a thermostat dep becomes disabled, blocked, faulted, ready, or is reassigned
- **THEN** realtime device snapshots for the affected thermostat reflect the updated dep and effective status without requiring a full page reload

#### Scenario: Quiet check does not publish
- **WHEN** a thermostat check completes without changing visible thermostat output, dep status, lifecycle status, config revision, or switch state
- **THEN** the firmware does not emit a realtime device update solely for that check

#### Scenario: Frontend store merges thermostat updates
- **WHEN** the SPA receives a realtime thermostat device snapshot
- **THEN** it updates dep fields, mode, config, output state, lifecycle status, and effective status from the payload alone
