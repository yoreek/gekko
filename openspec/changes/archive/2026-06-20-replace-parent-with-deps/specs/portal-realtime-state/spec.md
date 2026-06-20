## ADDED Requirements

### Requirement: Realtime snapshots expose deps
The firmware SHALL publish canonical realtime device snapshots using `deps` and computed `has_deps`.

#### Scenario: Device update includes deps
- **WHEN** a device realtime snapshot is published
- **THEN** the payload includes the same `deps` and computed `has_deps` fields used by REST snapshots

#### Scenario: Parent fields are absent from realtime
- **WHEN** a device realtime snapshot is published after the dependency migration
- **THEN** the payload does not include `has_parent` or `parent_device_id`

#### Scenario: Frontend merges deps
- **WHEN** the frontend receives a realtime device snapshot
- **THEN** it updates the device store dependency fields from the payload alone
