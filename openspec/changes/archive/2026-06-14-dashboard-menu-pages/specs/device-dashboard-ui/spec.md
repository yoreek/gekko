## MODIFIED Requirements

### Requirement: Device dashboard cards
The SPA SHALL present the device registry as the home dashboard of device cards that summarize the common operational state of each device. The home dashboard SHALL not include WiFi, OTA, system, or controller overview sections.

#### Scenario: Device cards are rendered on the dashboard
- **WHEN** the home dashboard loads device registry data
- **THEN** it renders one card per device with the shared base fields visible at a glance

#### Scenario: Non-device sections are absent from the dashboard
- **WHEN** the home dashboard is displayed
- **THEN** WiFi, OTA, system, and controller overview panels are not rendered on the same page

#### Scenario: Cards show common state
- **WHEN** a device card is displayed
- **THEN** it shows at least `device_id`, `type`, `name`, `status`, `enabled`, `registry_revision`, `config_revision`, and `pending_persistence`

#### Scenario: Cards reflect live updates
- **WHEN** a realtime device update arrives
- **THEN** the affected card updates without requiring a full page reload
