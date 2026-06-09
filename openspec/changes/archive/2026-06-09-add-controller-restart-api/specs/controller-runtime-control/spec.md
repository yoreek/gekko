## ADDED Requirements

### Requirement: Runtime restart API endpoint
The firmware SHALL provide an HTTP API endpoint that requests a controlled controller restart for operational runtime control.

#### Scenario: Restart endpoint is available in allowed profile
- **WHEN** a build profile enables runtime control restart API support
- **THEN** the firmware exposes a restart endpoint under the portal API namespace

#### Scenario: Restart endpoint is not available in restricted profile
- **WHEN** a build profile does not enable runtime control restart API support
- **THEN** the firmware does not register the restart endpoint

### Requirement: Safe restart transaction
The firmware SHALL execute restart requests as a safe transaction with pre-restart persistence flush and deterministic API responses.

#### Scenario: Restart request succeeds
- **WHEN** a client sends a valid restart request and pre-restart flush succeeds
- **THEN** the firmware returns a success response indicating reboot is in progress, closes the client connection, and then restarts the controller

#### Scenario: Restart request fails pre-restart flush
- **WHEN** a client sends a valid restart request and `DeviceRegistry::flushNow()` fails
- **THEN** the firmware returns an error response and does not restart the controller

#### Scenario: Restart request keeps portal flow non-blocking
- **WHEN** restart handling performs flush and response operations
- **THEN** the firmware avoids introducing long blocking loops outside existing bounded flush and async response flow
