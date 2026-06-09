## ADDED Requirements

### Requirement: Controller configuration remains separate from dynamic device registry
The firmware SHALL keep controller-level configuration separate from the dynamic device registry while allowing both to use NVS-backed persistence.

#### Scenario: Controller configuration is loaded
- **WHEN** the firmware loads controller-level configuration for identity, WiFi, provisioning, or firmware update behavior
- **THEN** the firmware does not require dynamic device registry data to be present or valid

#### Scenario: Dynamic registry changes
- **WHEN** a caller creates, updates, disables, or deletes a dynamic device
- **THEN** the firmware persists the dynamic device registry without changing controller-level configuration fields

#### Scenario: Registry storage format does not constrain controller config
- **WHEN** the dynamic device registry uses an index plus per-device records in NVS
- **THEN** controller-level configuration may continue using its existing typed NVS keys or other format selected for controller settings

#### Scenario: Controller configuration changes
- **WHEN** a caller changes controller-level configuration such as WiFi credentials or provisioning settings
- **THEN** the firmware persists those controller settings without rewriting dynamic device records unless an explicit registry migration is required
