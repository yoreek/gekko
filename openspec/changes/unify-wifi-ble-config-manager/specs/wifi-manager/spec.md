## MODIFIED Requirements

### Requirement: Espressif-compatible BLE mobile provisioning
The firmware SHALL support BLE WiFi credential entry as a managed WiFi configuration mode owned by `WifiManager`.

#### Scenario: Mobile provisioning session starts
- **WHEN** BLE config mode is requested and mobile provisioning is enabled
- **THEN** `WifiManager` starts the BLE provisioning service with declared security settings and device identity

#### Scenario: Mobile app provides credentials
- **WHEN** a compatible mobile app submits WiFi credentials through the provisioning protocol
- **THEN** the firmware records the received credentials, stops provisioning, deinitializes provisioning resources, persists the credentials through the configuration layer, and attempts to connect as a station through `WifiManager`

#### Scenario: Mobile provisioning can be disabled
- **WHEN** the build or runtime configuration disables mobile provisioning
- **THEN** the firmware still supports SoftAP and HTTP portal provisioning

#### Scenario: Real phone provisioning is verified
- **WHEN** the BLE credential entry milestone is tested
- **THEN** a real Android or iOS phone can discover the device, submit WiFi credentials, and the firmware can observe the credential-received event or a diagnosable failure

#### Scenario: Provisioning session is timeout-driven
- **WHEN** a BLE config session is active
- **THEN** session progress, timeout, credential receipt, stop, and deinit handling are coordinated by non-blocking `WifiManager` state-machine flow

## ADDED Requirements

### Requirement: Single WiFi stack owner
The firmware SHALL route station, AP, scan, BLE config, credential persistence, and provisioning lifecycle decisions through `WifiManager`.

#### Scenario: Portal submits credentials
- **WHEN** the portal receives WiFi credentials
- **THEN** the portal passes them directly to `WifiManager` for validation, persistence, and connection scheduling

#### Scenario: Portal requests BLE config mode
- **WHEN** the portal requests BLE config mode
- **THEN** the portal passes the request directly to `WifiManager` and does not use a provisioning coordinator

#### Scenario: WiFi stack commands are issued
- **WHEN** station, AP, or provisioning state changes require platform WiFi actions
- **THEN** those actions are initiated from `WifiManager` state-machine states through the platform driver or explicit provisioning calls

### Requirement: Idle-centered station and AP flow
The firmware SHALL use `Idle` as the only normal runtime state that chooses between station connection and setup AP mode.

#### Scenario: Idle chooses station connection
- **WHEN** stored credentials exist
- **THEN** `Idle` transitions to `Connecting`

#### Scenario: Idle chooses setup AP
- **WHEN** stored credentials do not exist
- **THEN** `Idle` transitions to `SetupAp`

#### Scenario: Station connection attempt starts
- **WHEN** `Connecting` is entered
- **THEN** it starts station connection with the current credentials and transitions to `CheckConnection`

#### Scenario: Station connection attempt succeeds
- **WHEN** `CheckConnection` observes connected driver status
- **THEN** it transitions to `Connected`

#### Scenario: Portal credentials are submitted during station wait
- **WHEN** `CheckConnection` observes pending portal-submitted credentials
- **THEN** it transitions to `ApplySubmittedCredentials`

#### Scenario: BLE config is requested during station wait
- **WHEN** `CheckConnection` has not completed and BLE config mode is requested
- **THEN** it transitions to `StartBleConfig` without waiting for the connection timeout

#### Scenario: Station connection attempt times out
- **WHEN** `CheckConnection` reaches the configured connection timeout before connected status is observed
- **THEN** it increments the retry count and transitions to `RetryDelay` if retries remain, otherwise to `SetupAp`

#### Scenario: BLE config is requested during retry delay
- **WHEN** `RetryDelay` is waiting and BLE config mode is requested
- **THEN** it transitions to `StartBleConfig` without waiting for the retry delay timeout

#### Scenario: Retry delay completes
- **WHEN** `RetryDelay` reaches the configured retry delay
- **THEN** it transitions to `Idle`

#### Scenario: BLE config is requested during setup AP
- **WHEN** `SetupAp` is active and BLE config mode is requested
- **THEN** it transitions to `StartBleConfig`

#### Scenario: BLE config is requested while connected
- **WHEN** `Connected` is active and BLE config mode is requested
- **THEN** it transitions to `StartBleConfig`

#### Scenario: Connected station disconnects
- **WHEN** `Connected` observes that the station is no longer connected
- **THEN** it resets retry state and transitions to `Idle`

#### Scenario: Command flows complete
- **WHEN** credential submit or BLE config completion states finish their action
- **THEN** they transition to `Idle` for the next normal runtime choice

### Requirement: SDK credentials are not the persistent source of truth
The firmware SHALL keep persistent WiFi credentials in `ConfigStore` rather than relying on SDK WiFi storage.

#### Scenario: WiFi driver begins
- **WHEN** the platform WiFi driver is initialized
- **THEN** it disables SDK WiFi credential persistence and Arduino WiFi auto reconnect before starting WiFi modes or connection attempts

#### Scenario: Station connection starts
- **WHEN** `WifiManager` starts a station connection
- **THEN** it supplies credentials explicitly from firmware configuration rather than using `WiFi.begin()` without arguments

### Requirement: WiFi event callbacks are state-machine inputs
The firmware SHALL use WiFi/provisioning event callbacks for logging and bounded event capture, not for direct lifecycle actions.

#### Scenario: Provisioning credentials event arrives
- **WHEN** `ARDUINO_EVENT_PROV_CRED_RECV` is received
- **THEN** the event handler logs the event, copies the credentials into bounded storage, and sets a flag consumed by `WifiManager::tick()`

#### Scenario: Provisioning end event arrives
- **WHEN** `ARDUINO_EVENT_PROV_END` is received
- **THEN** the event handler logs the event and sets a flag consumed by `WifiManager::tick()`

#### Scenario: State transition is needed
- **WHEN** a captured event requires a state transition
- **THEN** `WifiManager::tick()` performs the transition and any associated calls such as stop, deinit, save, disconnect, or connect
