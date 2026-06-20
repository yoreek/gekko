## ADDED Requirements

### Requirement: Thermostat realtime updates
The firmware SHALL publish thermostat runtime output, dep status, and lifecycle changes through the existing canonical device realtime topics.

#### Scenario: Control change publishes snapshot
- **WHEN** a thermostat runtime changes desired switch output, output status, or latest valid temperature state
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
