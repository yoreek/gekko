## MODIFIED Requirements

### Requirement: Device update topics carry nested bus diagnostics
The firmware SHALL publish canonical device snapshots that include nested bus diagnostics for supported bus runtimes.

#### Scenario: Realtime snapshot includes diagnostics
- **WHEN** the firmware publishes a realtime snapshot for a supported I2C or SPI bus device
- **THEN** the payload includes the same nested `runtime.diagnostics` object used by REST snapshots

#### Scenario: Realtime snapshot remains mergeable
- **WHEN** the frontend receives a bus device snapshot with diagnostics
- **THEN** it can merge the payload directly into the device store without a separate refresh

#### Scenario: Diagnostics stay runtime-only in realtime
- **WHEN** the firmware publishes a bus device snapshot over WebSocket
- **THEN** the diagnostic fields appear only under `runtime.diagnostics` and not under `config`

### Requirement: Bus diagnostics updates are coalesced in realtime
The firmware SHALL avoid broadcasting a separate realtime message for every low-level diagnostic increment when the visible bus snapshot can be published as one consolidated update.

#### Scenario: Error bursts coalesce
- **WHEN** a bus runtime records several errors in quick succession
- **THEN** the firmware updates the in-memory counters immediately but publishes a consolidated snapshot after the debounce window

#### Scenario: Reset publishes immediately
- **WHEN** diagnostics are reset on a supported bus device
- **THEN** the firmware broadcasts the updated device snapshot immediately

