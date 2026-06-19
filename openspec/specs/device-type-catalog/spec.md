## Purpose

Define the portal's canonical numeric device type catalog and localized labels.

## Requirements

### Requirement: Supported device type catalog
The portal SHALL define a canonical numeric catalog of supported device types for the device dashboard and create flow.

#### Scenario: Catalog contains the supported DummyDevice type
- **WHEN** the frontend renders the device type catalog
- **THEN** it exposes `DummyDevice` with `type_id = 1` as a supported catalog entry

#### Scenario: Catalog contains the GPIO switch type
- **WHEN** the frontend renders the device type catalog after GPIO switch support is added
- **THEN** it exposes `GpioSwitchDevice` with a stable numeric `type_id = 2` as a supported catalog entry

#### Scenario: GPIO switch metadata is available
- **WHEN** the frontend builds type-specific forms or widgets for `GpioSwitchDevice`
- **THEN** the catalog exposes the label key, local icon key, and component registry key required to resolve GPIO switch UI components without installing icon packages

#### Scenario: Create flow uses the catalog entry
- **WHEN** a user creates a device from the dashboard
- **THEN** the portal sends the selected numeric `type_id` from the catalog in the create request

### Requirement: Device type labels are localized
The portal SHALL resolve device type display labels from locale keys while keeping the numeric `type_id` as the underlying identifier.

#### Scenario: Label is shown in the active locale
- **WHEN** the dashboard renders the supported device type in English or Russian
- **THEN** it displays the localized label for `DummyDevice` while preserving the numeric `type_id` in data

#### Scenario: Catalog remains stable across reloads
- **WHEN** the user refreshes the SPA or opens a new dashboard route
- **THEN** the same numeric catalog entry remains available without requiring a dynamic lookup

### Requirement: DS18B20 device type catalog entry
The portal SHALL expose DS18B20 as a supported dynamic device type with stable numeric metadata and localized labels.

#### Scenario: Catalog contains DS18B20 type
- **WHEN** the frontend renders the device type catalog
- **THEN** it exposes `Ds18b20TemperatureSensorDevice` with stable `type_id = 4`

#### Scenario: DS18B20 metadata is available
- **WHEN** the frontend builds DS18B20 forms, details, or widgets
- **THEN** the catalog exposes the DS18B20 label key, local icon key, and component registry key without requiring a dynamic backend lookup

#### Scenario: DS18B20 label is localized
- **WHEN** the dashboard renders the DS18B20 type in English or Russian
- **THEN** it displays the localized DS18B20 label while preserving numeric `type_id = 4` in data and API payloads
