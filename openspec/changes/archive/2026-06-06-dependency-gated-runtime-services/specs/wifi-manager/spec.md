## MODIFIED Requirements

### Requirement: HTTP WiFi configuration portal
The firmware SHALL provide an asynchronous HTTP portal that supports WiFi scanning, network selection, credential entry, and saving setup settings, and SHALL start the HTTP backend only after required WiFi/network stack dependencies are ready.

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

#### Scenario: Portal backend waits for network stack readiness
- **WHEN** the portal service is configured but the WiFi/network stack is not ready for AsyncTCP or HTTP server startup
- **THEN** the portal remains in a dependency-wait state and does not call the async server backend `begin()`

#### Scenario: Portal backend starts from service tick
- **WHEN** the WiFi/network stack becomes ready during cooperative runtime
- **THEN** the portal service starts the async HTTP backend from its own tick-driven lifecycle without requiring `App::begin()` to tick WiFi first

#### Scenario: Portal survives AP and station mode transitions
- **WHEN** WiFi switches between setup AP, station, or AP+station modes while the WiFi/network stack remains initialized
- **THEN** the portal HTTP backend remains running and reachable through whichever interface has valid network addressing

#### Scenario: Portal restarts after WiFi stack stop
- **WHEN** WiFi is explicitly stopped or placed into `WIFI_OFF`, `WIFI_MODE_NULL`, or another down mode after the portal HTTP backend has started
- **THEN** the portal service stops or faults the HTTP backend and starts it again only after WiFi/network stack readiness returns

### Requirement: Captive portal discovery
The firmware SHALL make the setup portal reachable through direct IP access and best-effort captive portal DNS/HTTP redirection, with DNS startup gated by setup AP readiness.

#### Scenario: Direct setup page is available
- **WHEN** a client connects to the setup access point and opens the device IP address
- **THEN** the setup portal responds with the configuration interface

#### Scenario: Captive detection request is redirected
- **WHEN** a connected client sends a common captive portal detection request
- **THEN** the firmware redirects or responds so the user can reach the setup portal when the client platform permits it

#### Scenario: Captive DNS waits for setup AP address
- **WHEN** the HTTP portal is running but setup AP is inactive or has no valid AP IP address
- **THEN** the captive DNS backend remains stopped

#### Scenario: Captive DNS follows setup AP availability
- **WHEN** setup AP becomes active with a valid AP IP address or later becomes unavailable
- **THEN** the captive DNS backend starts or stops from the portal service tick without blocking the loop

#### Scenario: Setup AP readiness requires successful AP start
- **WHEN** setup AP startup fails or no valid setup AP IP address is available
- **THEN** setup AP readiness remains false and captive DNS is not started
