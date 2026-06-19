## ADDED Requirements

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
