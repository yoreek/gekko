## Purpose

Define WiFi station, provisioning, and recovery behavior for the firmware.

## Requirements

### Requirement: First-boot provisioning mode
The firmware SHALL enter provisioning mode when no valid WiFi station credentials are stored.

#### Scenario: No credentials on boot
- **WHEN** the device boots and configuration storage contains no valid WiFi credentials
- **THEN** the device starts a provisioning access point and makes the HTTP configuration interface available

#### Scenario: Credentials exist on boot
- **WHEN** the device boots and valid WiFi credentials are stored
- **THEN** the device attempts station-mode connection before deciding whether setup AP mode is needed

### Requirement: Provisioning access point
The firmware SHALL provide a temporary SoftAP for local setup with a deterministic device-specific SSID.

#### Scenario: Setup AP is started
- **WHEN** provisioning mode starts
- **THEN** the device broadcasts a setup SSID that includes a stable device identifier derived from hardware identity or configured device name

#### Scenario: Setup AP remains policy-driven after successful station connection
- **WHEN** valid credentials are saved and the device connects to the selected WiFi network
- **THEN** the setup AP remains governed by the WiFi state machine policy instead of being forced off by the station-connect path

### Requirement: HTTP WiFi configuration portal
The firmware SHALL provide an asynchronous HTTP portal that supports WiFi scanning, network selection, credential entry, and saving setup settings while remaining usable offline from setup AP mode.

#### Scenario: Networks are scanned
- **WHEN** a user requests available WiFi networks from the portal
- **THEN** the firmware returns a bounded list of nearby access points including SSID, signal strength, security type, and channel when available

#### Scenario: Credentials are submitted
- **WHEN** a user submits an SSID and password through the portal
- **THEN** the firmware validates input bounds, stores the pending credentials through the configuration layer, and attempts to connect as a station

#### Scenario: Invalid portal input is rejected
- **WHEN** a portal request contains an empty SSID, oversized values, or malformed configuration
- **THEN** the firmware rejects the request without overwriting the last valid stored configuration

#### Scenario: Async server does not require blocking polling
- **WHEN** the HTTP portal is active
- **THEN** portal request handling uses the async server stack and does not depend on a long blocking `handleClient()` polling loop

#### Scenario: SPA is served in setup AP mode
- **WHEN** a client connects to the setup access point and opens the portal root URL
- **THEN** the firmware serves the bundled SPA from LittleFS without requiring internet access

### Requirement: Captive portal discovery
The firmware SHALL make the setup portal reachable through direct IP access and best-effort captive portal DNS/HTTP redirection.

#### Scenario: Direct setup page is available
- **WHEN** a client connects to the setup access point and opens the device IP address
- **THEN** the setup portal responds with the configuration interface

#### Scenario: Captive detection request is redirected
- **WHEN** a connected client sends a common captive portal detection request
- **THEN** the firmware redirects or responds so the user can reach the setup portal when the client platform permits it

### Requirement: Station connection management
The firmware SHALL manage WiFi station connection attempts, retries, fallback, and status reporting.

#### Scenario: Connection succeeds
- **WHEN** saved credentials are valid and the target network is reachable
- **THEN** the firmware connects as a station, obtains network addressing, and reports connected status

#### Scenario: Connection fails
- **WHEN** station connection attempts fail after configured retry limits
- **THEN** the firmware enters setup AP mode without deleting the last saved credentials

#### Scenario: Connection flow is non-blocking
- **WHEN** the firmware attempts to connect to a WiFi network
- **THEN** connection progress, retry timing, and setup AP decisions are advanced by cooperative state-machine ticks rather than a blocking wait loop

#### Scenario: Connection state machine is covered by Unity tests
- **WHEN** WiFi connection retry or fallback logic is changed
- **THEN** Unity tests cover state transitions using mocked time and mocked WiFi events where practical

### Requirement: Espressif-compatible BLE mobile provisioning
The firmware SHALL support mobile-app WiFi credential entry through an explicit Espressif-compatible BLE config mode owned by `WifiManager`.

#### Scenario: Mobile provisioning session starts
- **WHEN** BLE config mode is requested and mobile provisioning is enabled
- **THEN** the firmware starts the Espressif-compatible BLE provisioning service with declared security settings and device identity

#### Scenario: Mobile app provides credentials
- **WHEN** a compatible mobile app submits WiFi credentials through the provisioning protocol
- **THEN** the firmware records the credentials, persists them through the same configuration flow used by the HTTP portal, stops provisioning, deinitializes provisioning, and returns to the normal WiFi state-machine flow

#### Scenario: Mobile provisioning can be disabled
- **WHEN** the build or runtime configuration disables mobile provisioning
- **THEN** the firmware still supports SoftAP and HTTP portal provisioning

#### Scenario: Real phone provisioning is verified
- **WHEN** the first mobile provisioning milestone is tested
- **THEN** a real Android or iOS phone using an Espressif-compatible app can discover the device, submit WiFi credentials, and observe provisioning success or a diagnosable failure

#### Scenario: Provisioning session is timeout-driven
- **WHEN** a mobile provisioning session is active
- **THEN** session progress, timeout, credential receipt, stop, and deinit handling are coordinated by non-blocking state-machine flow

### Requirement: Provisioning timeout recovery
The firmware SHALL treat a mobile provisioning timeout as a recoverable state rather than a terminal dead end.

#### Scenario: Provisioning session times out
- **WHEN** a mobile provisioning session reaches its configured timeout without completing credential exchange
- **THEN** the firmware ends the session and allows provisioning to start again without requiring a device reboot
