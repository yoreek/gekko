## MODIFIED Requirements

### Requirement: HTTP WiFi configuration portal
The firmware SHALL provide an asynchronous HTTP portal that serves the bundled SPA, supports WiFi scanning, network selection, credential entry, and saving setup settings while remaining usable offline from setup AP mode.

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
