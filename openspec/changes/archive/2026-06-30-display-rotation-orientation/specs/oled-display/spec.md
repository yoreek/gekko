## ADDED Requirements

### Requirement: OLED runtime applies persisted display orientation
The OLED display runtime SHALL apply the persisted display orientation or rotation when initializing hardware and when rendering the active layout.

#### Scenario: Runtime initializes with stored rotation
- **WHEN** the OLED device starts
- **THEN** it applies the persisted rotation value to the hardware display driver before drawing the layout

#### Scenario: Runtime preserves layout rendering
- **WHEN** the display orientation changes
- **THEN** the runtime continues to render the same layout content in the new effective display space

### Requirement: OLED runtime tolerates legacy configs
The OLED runtime SHALL continue to load legacy device configs that predate the explicit orientation field by applying a safe default.

#### Scenario: Legacy config still boots
- **WHEN** an OLED device config does not include an explicit orientation value
- **THEN** the runtime loads it using the family default orientation

#### Scenario: Existing config remains stable
- **WHEN** a previously saved OLED device is reopened after migration
- **THEN** its orientation remains deterministic and matches the stored or migrated value
