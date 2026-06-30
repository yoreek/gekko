## ADDED Requirements

### Requirement: Display device configs persist orientation
Display device configuration SHALL persist the chosen orientation or rotation value as part of the versioned config schema.

#### Scenario: Configuration round-trips orientation
- **WHEN** the portal saves and reloads a display device config
- **THEN** the orientation value round-trips without being lost

#### Scenario: Missing orientation gets a safe default
- **WHEN** the firmware loads a legacy display config without an orientation field
- **THEN** it applies a safe family-specific default during migration or normalization

### Requirement: Orientation migration is backward compatible
The configuration migration flow SHALL preserve existing display behavior while introducing the new orientation field.

#### Scenario: Legacy config migrates safely
- **WHEN** an older supported display config is migrated to the current schema
- **THEN** the migration sets a deterministic orientation value that keeps the device renderable

#### Scenario: Config changes remain bounded
- **WHEN** a display config changes only by orientation
- **THEN** the update remains within the existing device config persistence path and does not require a separate storage format
