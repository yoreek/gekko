## ADDED Requirements

### Requirement: BLE config mode is state-machine controlled
The firmware SHALL provide a BLE WiFi credential entry mode that is started, stopped, and deinitialized by the cooperative `WifiManager` state machine.

#### Scenario: Config mode starts from the portal
- **WHEN** the portal requests BLE config mode
- **THEN** `WifiManager` starts the BLE credential service without handing WiFi runtime ownership to another domain manager

#### Scenario: Credentials are received
- **WHEN** the BLE credential service receives an SSID and password
- **THEN** the event handler records the credentials and `WifiManager` saves them before stopping BLE config mode on its next cooperative ticks

#### Scenario: Config mode times out
- **WHEN** BLE config mode exceeds the configured session timeout without receiving credentials
- **THEN** `WifiManager` stops and deinitializes BLE config mode and returns to normal WiFi flow without changing stored credentials

### Requirement: BLE provisioning shutdown ordering
The firmware SHALL stop provisioning before deinitializing provisioning resources.

#### Scenario: Credentials trigger shutdown
- **WHEN** BLE credentials are received
- **THEN** `WifiManager` saves the credentials, calls `wifi_prov_mgr_stop_provisioning()`, waits for `ARDUINO_EVENT_PROV_END`, and then calls `wifi_prov_mgr_deinit()`

#### Scenario: Timeout triggers shutdown
- **WHEN** BLE config mode times out
- **THEN** `WifiManager` calls `wifi_prov_mgr_stop_provisioning()`, waits for `ARDUINO_EVENT_PROV_END`, and then calls `wifi_prov_mgr_deinit()`

### Requirement: BLE credentials use firmware storage
The firmware SHALL persist credentials received through BLE using the same `ConfigStore` path as portal-submitted credentials.

#### Scenario: Credentials are accepted
- **WHEN** BLE config mode receives credentials that pass format validation
- **THEN** `WifiManager` saves them to `ConfigStore`, completes BLE stop/deinit, and starts the normal station connection flow with those credentials

#### Scenario: Saved credentials fail to connect
- **WHEN** credentials received through BLE fail after the configured station retry policy
- **THEN** the firmware enters AP mode without restoring previous credentials
