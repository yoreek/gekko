## ADDED Requirements

### Requirement: Device event publication
The firmware SHALL publish normalized events for dynamic device registry, lifecycle, status, configuration, and command outcomes.

#### Scenario: Device is created
- **WHEN** a dynamic device create request is accepted
- **THEN** the firmware emits a device-created event containing the registry revision, device ID, device type, name, enabled state, and current lifecycle status

#### Scenario: Device status changes
- **WHEN** a dynamic device runtime status changes
- **THEN** the firmware emits a status-changed event containing the device ID, previous status, new status, and bounded status detail

#### Scenario: Device is deleted
- **WHEN** a dynamic device delete request is accepted
- **THEN** the firmware emits a device-deleted event containing the deleted device ID and registry revision

### Requirement: Integration adapters
The firmware SHALL expose a transport-neutral integration interface that concrete MQTT, WebSocket, Home Assistant, or future adapters can implement.

#### Scenario: Integration receives event
- **WHEN** the registry publishes a device event
- **THEN** each registered integration adapter is notified through the common interface without the registry depending on a transport-specific class

#### Scenario: Integration performs network work
- **WHEN** an integration adapter needs to publish over a network transport
- **THEN** it performs transport work from an App-scheduled cooperative tick cadence and does not block the registry mutation path

#### Scenario: Integration needs globally unique identity
- **WHEN** an integration such as Home Assistant needs a globally unique object or entity identifier
- **THEN** the firmware derives it from controller identity plus the device ID instead of requiring each dynamic device to store a UUID

### Requirement: Bounded event pipeline
The firmware SHALL route dynamic device events through a bounded, cooperative event pipeline before fanout to registered integration sinks.

#### Scenario: Registry enqueues event
- **WHEN** the registry accepts a dynamic device event
- **THEN** the firmware places the event into a fixed-size queue instead of allocating unbounded memory or performing network work inline

#### Scenario: Dispatcher drains queued events
- **WHEN** the application scheduler runs the event dispatcher on a cooperative tick
- **THEN** the firmware drains queued events in order and fans them out to registered sinks without blocking the mutation path

#### Scenario: Queue overflows
- **WHEN** the bounded event queue is full
- **THEN** the firmware rejects or drops the new event according to the bounded event policy and keeps the cooperative loop responsive

### Requirement: External command normalization
The firmware SHALL normalize commands received from Web UI and integration transports before applying them to dynamic devices.

#### Scenario: Integration command is accepted
- **WHEN** an integration submits a valid command to create, update, delete, enable, disable, rename, or control a device
- **THEN** the firmware applies the same registry validation used by the Web UI and emits or returns a command-accepted result that indicates whether persistence is pending

#### Scenario: Integration command is rejected
- **WHEN** an integration submits an invalid command or a command not supported by the target device type
- **THEN** the firmware rejects the command, leaves registry state unchanged, and emits or returns a command-rejected result with a bounded error reason

### Requirement: Event ordering and revisions
The firmware SHALL attach monotonic registry or event revision information to dynamic device events.

#### Scenario: Multiple mutations occur
- **WHEN** multiple registry mutations are accepted
- **THEN** the firmware emits events with revision values that allow integrations to observe the accepted mutation order

#### Scenario: Config mutation event is emitted
- **WHEN** a device configuration mutation is accepted
- **THEN** the firmware emits an event containing the registry revision, the target device config revision, and whether persistence is pending

#### Scenario: Status event occurs without registry mutation
- **WHEN** a runtime-only status event occurs without changing persisted configuration
- **THEN** the firmware emits the event without rewriting device configuration NVS, without incrementing config revision, and preserves enough ordering information for integrations to process it after prior registry events

#### Scenario: Persistence completion event is emitted
- **WHEN** a delayed dirty index, config record, or retained-state record is successfully written to NVS
- **THEN** the firmware may emit a bounded persisted event or update the next event/API response so integrations can observe that pending persistence cleared

#### Scenario: Retained state event occurs
- **WHEN** a retained runtime state value is persisted separately from device configuration
- **THEN** the firmware may emit a bounded retained-state event that identifies the device ID without treating the retained value as a configuration mutation

### Requirement: Bounded event payloads
The firmware SHALL keep integration events bounded for ESP32 memory constraints.

#### Scenario: Event detail is too large
- **WHEN** a device, command, or status detail exceeds the supported event payload size
- **THEN** the firmware truncates or rejects the detail according to event type and reports a bounded error instead of allocating an unbounded buffer

#### Scenario: Integration is unavailable
- **WHEN** a registered integration cannot currently publish events
- **THEN** the firmware keeps the cooperative loop responsive and lets the integration drop, coalesce, or retry events according to its own bounded policy
