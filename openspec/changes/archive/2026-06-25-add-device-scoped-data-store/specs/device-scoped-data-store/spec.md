## ADDED Requirements

### Requirement: Device-scoped data is keyed by device ID and type
The firmware SHALL persist device-owned auxiliary data in a device-scoped storage area addressed by stable `DeviceId` plus a named data type.

#### Scenario: Retained state is saved and loaded by type
- **WHEN** a device saves `retained_state` and later loads `retained_state` for the same device ID
- **THEN** the firmware returns the saved payload without requiring the caller to use a device config blob

#### Scenario: Different data types stay independent
- **WHEN** a device saves `retained_state` and also saves `display_layout`
- **THEN** loading one type does not overwrite or clear the other type

### Requirement: Device-scoped data can be cleared per device
The firmware SHALL clear all device-scoped data for a device when the owning device is deleted.

#### Scenario: Deleted device loses scoped data
- **WHEN** the registry deletes a device
- **THEN** all device-scoped data for that device becomes unavailable on the next load attempt

#### Scenario: Cleanup does not affect other devices
- **WHEN** device-scoped data is cleared for one device ID
- **THEN** data for other device IDs remains available

### Requirement: Legacy retained state migrates into device-scoped storage
The firmware SHALL migrate legacy retained-state records into the device-scoped data mechanism and remove the legacy record after a successful migration write.

#### Scenario: Legacy retained state loads through migration
- **WHEN** a device loads retained state and only the legacy retained-state record exists
- **THEN** the firmware reads the legacy value, writes it into the new device-scoped location, and returns the migrated value

#### Scenario: Legacy retained state is preserved on write failure
- **WHEN** migration to the new device-scoped location fails
- **THEN** the firmware leaves the legacy retained-state record intact and reports the failure

### Requirement: Device-scoped data types remain isolated
The firmware SHALL treat each data type as an independent payload so a write to one type does not modify another type unless the caller explicitly clears the device scope.

#### Scenario: Write to one type does not remove another
- **WHEN** a device updates `display_layout`
- **THEN** the stored `retained_state` for that device remains unchanged

#### Scenario: Clear scope removes all types
- **WHEN** the caller clears the full device scope
- **THEN** all data types for that device are removed together
