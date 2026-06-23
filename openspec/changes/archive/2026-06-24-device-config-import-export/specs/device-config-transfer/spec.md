## ADDED Requirements

### Requirement: Versioned device setup transfer bundle
The system MUST export and import device configuration as a versioned JSON Lines transfer bundle that can represent a complete device setup.

#### Scenario: Bundle includes version metadata
- **WHEN** the firmware exports a device setup
- **THEN** the bundle begins with an explicit transfer schema version and enough metadata to validate future imports

#### Scenario: Unsupported bundle version is rejected
- **WHEN** the firmware receives a bundle with an unsupported transfer schema version
- **THEN** the import is rejected and no device state is changed

#### Scenario: Export emits JSON Lines records
- **WHEN** the firmware exports a device setup
- **THEN** the bundle is emitted as a sequence of JSON Lines records that can be consumed incrementally

### Requirement: Export redacts secrets and preserves restore data
The system MUST export the current device setup with stable device identity, dependency links, and type configuration metadata while excluding or redacting secrets from normal exports.

#### Scenario: Export contains restore data
- **WHEN** the user exports device configuration
- **THEN** the bundle includes flat device records with top-level IDs, device types, dependency links, revision metadata, supported config version data, and device-specific configuration fields required for restore

#### Scenario: Export does not emit binary config blobs
- **WHEN** the user exports device configuration
- **THEN** the bundle does not include a separate binary `config_blob_hex` field for device records

#### Scenario: Secrets are not exported by default
- **WHEN** the exported setup contains WiFi credentials or other secrets
- **THEN** the normal export omits or redacts those secrets

### Requirement: Import applies a validated snapshot atomically
The system MUST validate the complete transfer bundle before applying it and MUST treat a successful import as a restore snapshot rather than a partial patch.

#### Scenario: Valid bundle is restored
- **WHEN** the user imports a valid bundle within the accepted size limit
- **THEN** the firmware validates the full bundle, applies the restored device setup, and updates persisted state only after validation succeeds

#### Scenario: Invalid bundle does not change live state
- **WHEN** the user imports malformed JSON, oversized data, or a bundle that fails validation
- **THEN** the firmware rejects the import and leaves the current registry and configuration unchanged

#### Scenario: Replace restore is deterministic
- **WHEN** a valid bundle is restored
- **THEN** the resulting device setup matches the bundle content for device identity, dependencies, and supported configuration versions

#### Scenario: Import consumes uploaded bundle file
- **WHEN** the user imports a bundle through the portal API
- **THEN** the firmware reads the upload from the request file context rather than requiring the full request body in memory

### Requirement: Portal API exposes device setup transfer endpoints
The portal API MUST expose dedicated REST endpoints for device setup export and import.

#### Scenario: Export endpoint returns a bundle
- **WHEN** a client requests device setup export
- **THEN** the API returns the transfer bundle for download or direct consumption

#### Scenario: Import endpoint accepts a bundle
- **WHEN** a client submits a device setup bundle to the import endpoint
- **THEN** the API validates the uploaded bundle file and returns a success or error response consistent with the portal controller contract

### Requirement: Devices page exposes transfer actions
The portal SPA MUST expose export and import actions for device setup transfer on the Devices page.

#### Scenario: Export action is available
- **WHEN** the user opens the Devices page
- **THEN** the page provides an action to export the current device setup

#### Scenario: Import action uses a file chooser
- **WHEN** the user starts an import from the Devices page
- **THEN** the page lets the user select a transfer bundle file and confirms the restore action before sending it to the API
