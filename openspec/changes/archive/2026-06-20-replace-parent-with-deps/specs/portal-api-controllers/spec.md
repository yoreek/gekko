## MODIFIED Requirements

### Requirement: Device command requests use structured fields
The portal API SHALL expose `/api/devices/:id/command` requests as structured JSON commands with command-specific fields instead of a generic public string `payload`.

#### Scenario: Rename uses name field
- **WHEN** a client renames a device
- **THEN** the request uses `command = "rename"` with a string `name` field and does not use `payload` for the new name

#### Scenario: Set status uses status field
- **WHEN** a client sets a test or debug device status
- **THEN** the request uses `command = "set_status"` with a string `status` field and does not use `payload` for the status value

#### Scenario: Set deps uses dependency fields
- **WHEN** a client changes device dependency relationships
- **THEN** the request uses `command = "set_deps"` with a structured `deps` array rather than parent fields or a packed payload string

#### Scenario: Legacy command payload is rejected for migrated commands
- **WHEN** a client sends a migrated command using only `payload` instead of the required named field
- **THEN** the API rejects the request with the standard error envelope before mutating the registry or runtime

## ADDED Requirements

### Requirement: Device snapshots expose deps
The portal API SHALL expose device dependency links as `deps` plus computed `has_deps` in canonical device snapshots.

#### Scenario: Snapshot includes deps
- **WHEN** the API serializes a device with dependency links
- **THEN** the snapshot includes `deps` entries with role and device id

#### Scenario: Snapshot computes has deps
- **WHEN** the API serializes any device
- **THEN** `has_deps` is computed from the serialized `deps` array

#### Scenario: Parent fields are absent
- **WHEN** the API serializes a device after the dependency migration
- **THEN** it does not include `has_parent` or `parent_device_id`

#### Scenario: Delete rejection reports dependents
- **WHEN** a delete request is rejected because other devices depend on the target
- **THEN** the error response reports `dependent_device_ids`
