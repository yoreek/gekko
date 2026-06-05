## ADDED Requirements

### Requirement: Provisioning lifecycle is observable
The system SHALL emit explicit provisioning lifecycle logs when BLE or SoftAP provisioning starts and ends.

#### Scenario: BLE provisioning starts
- **WHEN** the device enters provisioning mode using the BLE transport
- **THEN** the system SHALL log a provisioning start event

#### Scenario: Provisioning ends
- **WHEN** the provisioning service stops for any reason
- **THEN** the system SHALL log a provisioning end event

### Requirement: Admin can re-enter BLE provisioning without clearing NVS
The system SHALL provide an admin control path that re-enters BLE provisioning mode without erasing stored WiFi credentials or resetting NVS.

#### Scenario: Re-enter from admin control
- **WHEN** an administrator triggers the provisioning re-entry action from the portal
- **THEN** the system SHALL stop the current provisioning session if needed, move the network layer into provisioning fallback AP mode if necessary, and start BLE provisioning again

#### Scenario: Stored configuration is preserved
- **WHEN** BLE provisioning is re-entered through the admin control path
- **THEN** the system SHALL preserve existing NVS-backed configuration data

### Requirement: Provisioning re-entry is accessible from the portal
The system SHALL expose the provisioning re-entry control through the existing admin portal.

#### Scenario: Portal exposes control
- **WHEN** the admin portal is available
- **THEN** the system SHALL expose a visible control or endpoint for re-entering BLE provisioning
