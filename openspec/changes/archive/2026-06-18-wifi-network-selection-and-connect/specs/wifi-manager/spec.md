## ADDED Requirements

### Requirement: WiFi credential reset returns the station flow to AP readiness
The firmware SHALL allow stored WiFi credentials to be cleared and SHALL let the existing station connection states observe the empty runtime credential set so the cooperative flow falls back to setup AP without a manual state override.

#### Scenario: Credential reset clears the runtime cache
- **WHEN** the firmware accepts a WiFi credential reset request
- **THEN** the WiFi manager clears the stored WiFi credentials and clears its runtime credential cache so the station branch no longer sees a valid SSID

#### Scenario: Station connection states observe empty credentials
- **WHEN** the WiFi manager is in `CheckConnection` or `Connected` and the runtime credential cache becomes empty
- **THEN** the next cooperative tick leaves the station branch, returns through `Idle`, and then enters `SetupAp` through the existing idle fallback logic

#### Scenario: Reset does not require a manual state jump
- **WHEN** WiFi credentials are cleared while the station branch is active
- **THEN** the firmware does not require a direct manual state override to make AP mode available again

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

#### Scenario: Empty credentials return the machine to AP fallback
- **WHEN** the WiFi manager clears its runtime credential cache while the station branch is active
- **THEN** the existing station guards no longer keep the device in STA mode and the cooperative flow falls back to setup AP through `Idle`

#### Scenario: Connection state machine is covered by Unity tests
- **WHEN** WiFi connection retry or fallback logic is changed
- **THEN** Unity tests cover state transitions using mocked time and mocked WiFi events where practical
