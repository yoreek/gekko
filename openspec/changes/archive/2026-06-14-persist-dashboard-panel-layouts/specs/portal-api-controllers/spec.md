## MODIFIED Requirements

### Requirement: Existing portal API routes are preserved
The firmware SHALL preserve the existing first-milestone REST endpoint paths and response semantics while migrating route implementation to controllers, and SHALL add dashboard layout routes without changing existing route behavior.

#### Scenario: WiFi status endpoint remains available
- **WHEN** a client requests `GET /api/wifi/status`
- **THEN** the migrated controller returns the same WiFi status contract as the existing portal route

#### Scenario: WiFi scan endpoint remains available
- **WHEN** a client requests `GET /api/wifi/scan`
- **THEN** the migrated controller returns the same bounded scan result contract as the existing portal route

#### Scenario: Device registry endpoints remain available
- **WHEN** a client calls `/api/devices` with an existing supported method
- **THEN** the migrated controller preserves the existing path, validation behavior, response fields, and registry revision reporting

#### Scenario: Device command endpoint targets a device id
- **WHEN** a client sends `POST /api/devices/:id/command`
- **THEN** the migrated controller validates the path device id, applies the command to that device, and returns the standard success or error envelope with registry revision reporting when relevant

#### Scenario: OTA and system endpoints remain available
- **WHEN** a client requests `/api/ota/status` or sends `POST /api/system/restart`
- **THEN** the migrated controllers preserve the existing status and restart behavior

#### Scenario: Dashboard layout endpoint remains under API routing
- **WHEN** a client calls `/api/dashboard/layout`
- **THEN** the portal handles the request through a REST controller and does not serve the SPA fallback for that API path

## ADDED Requirements

### Requirement: Dashboard layout controller uses shared REST behavior
The firmware SHALL implement dashboard layout REST routes using the shared portal REST controller behavior for CORS, no-cache API headers, bounded JSON parsing, and success/error envelopes.

#### Scenario: Dashboard layout GET uses success envelope
- **WHEN** a client requests `GET /api/dashboard/layout`
- **THEN** the controller returns `success: true` with `revision` and `layout`

#### Scenario: Dashboard layout PUT validates JSON body
- **WHEN** a client sends `PUT /api/dashboard/layout`
- **THEN** the controller parses the request body through bounded shared JSON parsing before invoking the layout storage mutation

#### Scenario: Dashboard layout errors use shared error envelope
- **WHEN** a dashboard layout request is invalid
- **THEN** the controller returns `{"success":false,"code":"...","error":"..."}` with a stable machine-readable code

#### Scenario: Dashboard layout preflight is handled
- **WHEN** a client sends `OPTIONS /api/dashboard/layout`
- **THEN** the controller returns the shared no-content CORS preflight response without mutating stored layout
