## Purpose

Define the shared REST controller layer for the portal HTTP API.

## Requirements

### Requirement: Shared portal REST controller base
The firmware SHALL provide a shared controller base for portal REST endpoints that centralizes request dispatch, bounded JSON body parsing, JSON success/error responses, CORS headers, no-cache API headers, and `OPTIONS` preflight handling.

#### Scenario: CORS headers are applied
- **WHEN** a portal REST controller sends a JSON response
- **THEN** the response includes `Access-Control-Allow-Origin: *`, `Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS`, `Access-Control-Allow-Headers: Content-Type`, and `Access-Control-Max-Age: 3600`

#### Scenario: OPTIONS preflight is handled consistently
- **WHEN** a client sends an `OPTIONS` request to a portal REST endpoint
- **THEN** the controller returns a no-content response with CORS headers and does not invoke the endpoint mutation handler

#### Scenario: Invalid JSON is rejected
- **WHEN** a controller action that requires a JSON body receives malformed or oversized JSON input
- **THEN** the controller rejects the request with a bounded JSON error response and does not call the domain mutation

#### Scenario: Success envelope is consistent
- **WHEN** a portal REST controller sends a successful JSON response
- **THEN** the response includes `success` set to `true` plus endpoint-specific payload fields

#### Scenario: Error envelope is consistent
- **WHEN** a portal REST controller rejects a request
- **THEN** the response body uses `{"success":false,"code":"...","error":"..."}` with a stable machine-readable code and human-readable error string

### Requirement: Existing portal API routes are preserved
The firmware SHALL preserve the existing first-milestone REST endpoint paths and response semantics while migrating route implementation to controllers.

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

### Requirement: Streamed JSON for potentially large responses
The firmware SHALL stream REST responses that can grow with device count instead of building one large intermediate payload string.

#### Scenario: Device list is serialized incrementally
- **WHEN** a client requests the device registry list
- **THEN** the controller writes the response incrementally to an async response stream and avoids concatenating the full device list into a temporary `String`

#### Scenario: Empty list remains valid JSON
- **WHEN** a streamed list response contains no items
- **THEN** the controller returns syntactically valid JSON with an empty array and success metadata
