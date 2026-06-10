## MODIFIED Requirements

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

#### Scenario: Dashboard state remains synchronized
- **WHEN** a device list, selected device, or modal detail view is affected by a supported realtime message
- **THEN** the frontend updates the dashboard card list and the open modal in a consistent way without losing the active selection unless the selected device is deleted
