## ADDED Requirements

### Requirement: Canonical backend device record contract
The backend SHALL represent each dynamic device as a nested device record with separate identity, persisted config, and runtime state sections.

#### Scenario: API and realtime snapshots share the same record shape
- **WHEN** the backend serializes a device for REST or realtime delivery
- **THEN** it uses a nested device record with `record`, `config`, and `runtime` sections rather than duplicating identity or config fields at the top level

#### Scenario: Identity stays in the record wrapper
- **WHEN** the backend serializes a device record
- **THEN** `record` contains the stable device identity fields, while persisted settings remain inside `config` and live state remains inside `runtime`

### Requirement: Canonical device setup bundle contract
The backend SHALL export and import setup bundles using a nested device setup record that contains identity and persisted config only.

#### Scenario: Export omits runtime
- **WHEN** the backend exports a device setup bundle
- **THEN** each device entry contains `record` and `config` sections and does not include `runtime`

#### Scenario: Import restores persisted fields only
- **WHEN** the backend imports a device setup bundle
- **THEN** it accepts the nested `record` and `config` sections, restores the persisted device settings, and reconstructs runtime state later from registry startup

### Requirement: Persisted config fields stay inside config
The backend SHALL keep shared persisted fields such as `name`, `enabled`, and `deps` inside the device config object and SHALL not duplicate them on the record wrapper.

#### Scenario: Runtime changes do not rewrite persisted config fields
- **WHEN** a runtime-only status or output value changes
- **THEN** the backend updates runtime state without moving `name`, `enabled`, or `deps` out of `config`

#### Scenario: Config revisions remain on the record identity block
- **WHEN** a device configuration mutation is accepted
- **THEN** the backend updates the record revision metadata and leaves runtime output fields separate from the persisted config payload
