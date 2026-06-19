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

#### Scenario: Dashboard layout endpoint remains under API routing
- **WHEN** a client calls `/api/dashboard/layout`
- **THEN** the portal handles the request through a REST controller and does not serve the SPA fallback for that API path

### Requirement: Dashboard layout controller uses shared REST behavior
The firmware SHALL implement dashboard layout REST routes using the shared portal REST controller behavior for CORS, no-cache API headers, bounded JSON parsing, and success/error envelopes.

#### Scenario: Dashboard layout GET uses success envelope
- **WHEN** a client requests `GET /api/dashboard/layout`
- **THEN** the controller returns `success: true` with `revision` and `layout`

#### Scenario: Dashboard layout PUT validates JSON body
- **WHEN** a client sends `PUT /api/dashboard/layout`
- **THEN** the controller parses the request body through bounded shared JSON parsing before invoking the layout storage mutation

#### Scenario: Dashboard layout PUT returns no content on success
- **WHEN** a client sends a valid `PUT /api/dashboard/layout`
- **THEN** the controller persists the compact layout representation and returns `204 No Content`

#### Scenario: Dashboard layout errors use shared error envelope
- **WHEN** a dashboard layout request is invalid
- **THEN** the controller returns `{"success":false,"code":"...","error":"..."}` with a stable machine-readable code

#### Scenario: Dashboard layout preflight is handled
- **WHEN** a client sends `OPTIONS /api/dashboard/layout`
- **THEN** the controller returns the shared no-content CORS preflight response without mutating stored layout

#### Scenario: Dashboard layout keeps the compact widget contract
- **WHEN** a client reads or writes `/api/dashboard/layout`
- **THEN** the controller uses the compact tuple widget contract and does not require the old object-shaped widget payload

### Requirement: Dashboard layout storage is binary
The firmware SHALL store dashboard layout state as a compact binary blob and SHALL reset to a default layout when the binary data is absent or invalid instead of migrating old JSON snapshots.

#### Scenario: Valid layout is stored as binary
- **WHEN** the controller persists a valid dashboard layout
- **THEN** it writes the layout to the binary-backed storage representation and updates the revision

#### Scenario: Missing or invalid layout resets to default
- **WHEN** the controller loads dashboard layout state and the stored binary data is missing or invalid
- **THEN** it returns the default dashboard layout and does not preserve legacy JSON layout data

### Requirement: Streamed JSON for potentially large responses
The firmware SHALL stream REST responses that can grow with device count instead of building one large intermediate payload string.

#### Scenario: Device list is serialized incrementally
- **WHEN** a client requests the device registry list
- **THEN** the controller writes the response incrementally to an async response stream and avoids concatenating the full device list into a temporary `String`

#### Scenario: Empty list remains valid JSON
- **WHEN** a streamed list response contains no items
- **THEN** the controller returns syntactically valid JSON with an empty array and success metadata

### Requirement: Device snapshots expose switch runtime output
The portal API SHALL expose current switch-like runtime output state in device JSON snapshots without storing that state in the device config object.

#### Scenario: GPIO switch snapshot includes runtime output
- **WHEN** the API serializes a `GpioSwitchDevice` with a live runtime
- **THEN** the device JSON includes `output.state` with one of `on`, `off`, or `disabled`

#### Scenario: Runtime output is separate from config
- **WHEN** the API serializes a switch-like device snapshot
- **THEN** current runtime output state appears under `output` and not under `config`

#### Scenario: Config states remain persisted settings
- **WHEN** the API serializes a GPIO switch config
- **THEN** `config.startup_state` and `config.safe_state` remain persisted configuration settings independent from `output.state`

#### Scenario: Realtime snapshot includes runtime output
- **WHEN** a GPIO switch output state changes and the portal publishes a generic device update message
- **THEN** the message payload includes the same runtime `output.state` shape used by REST device snapshots

#### Scenario: No switch-specific topic is required
- **WHEN** the frontend observes GPIO switch output changes
- **THEN** it can update from the generic device update payload without subscribing to a dedicated switch topic

### Requirement: Type adapters can serialize runtime fields
The portal API SHALL pass the optional live runtime pointer to type-specific device API adapters when serializing device JSON.

#### Scenario: Adapter receives runtime context
- **WHEN** the controller serializes a device record and a live runtime exists
- **THEN** it calls the matching type adapter with both the persisted record and the runtime pointer

#### Scenario: Controller stays type-agnostic
- **WHEN** the controller serializes GPIO switch runtime output
- **THEN** the type-specific adapter writes the switch-specific `output` object without the controller branching on GPIO switch or switch types

#### Scenario: Missing runtime is tolerated
- **WHEN** a device record has no live runtime during serialization
- **THEN** the adapter still writes persisted fields and omits unavailable runtime-only output fields

### Requirement: DS18B20 device API contract
The portal API SHALL create, mutate, and serialize DS18B20 devices through the existing generic device registry endpoints.

#### Scenario: Create request includes parent and config
- **WHEN** a client creates a DS18B20 device through `POST /api/devices`
- **THEN** the request includes `type_id = 4`, common device fields, `has_parent = true`, `parent_device_id`, and a DS18B20 config object containing address, resolution, unit, poll period, report delta, and report policy

#### Scenario: Create rejects invalid parent or address
- **WHEN** a DS18B20 create request omits a compatible OneWire parent or contains an invalid DS18B20 address
- **THEN** the API returns the standard error envelope and does not create a partial registry record

#### Scenario: Update config can atomically update parent
- **WHEN** a client edits DS18B20 settings after creation
- **THEN** the SPA sends one `update_config` command to `POST /api/devices/:id/command` that can carry DS18B20 config and parent fields together, and the API validates the combined mutation before applying it

#### Scenario: Duplicate address is rejected
- **WHEN** a DS18B20 create or update request would duplicate an existing DS18B20 address on the same OneWire parent
- **THEN** the API returns the standard error envelope and does not change the registry

### Requirement: DS18B20 snapshot serialization
The portal API SHALL include DS18B20 persisted config, parent identity, lifecycle status, effective status, and latest temperature output in canonical device snapshots.

#### Scenario: Snapshot includes DS18B20 config
- **WHEN** the API serializes a DS18B20 device
- **THEN** the snapshot includes type `ds18b20_temperature_sensor`, parent fields, config address, resolution, unit, poll period, report delta, and report policy

#### Scenario: Snapshot includes valid temperature output
- **WHEN** the DS18B20 runtime has a valid reading
- **THEN** the snapshot includes `output.temperature` with numeric value, unit, unit symbol, measured timestamp, and valid state

#### Scenario: Snapshot tolerates missing reading
- **WHEN** the DS18B20 runtime has not produced a valid reading or the device is blocked
- **THEN** the snapshot remains valid JSON and includes `output.temperature.valid = false` for the unavailable reading

#### Scenario: Device list remains streamed
- **WHEN** the API streams the device registry list and DS18B20 devices are present
- **THEN** it serializes DS18B20 snapshots incrementally without concatenating the full device list into one temporary string

### Requirement: DS18B20 scan selection API support
The portal API SHALL expose enough OneWire scan data for DS18B20 selection while keeping scan execution on the parent OneWire bus device.

#### Scenario: Parent scan result remains generic
- **WHEN** the API serializes a OneWire bus scan result
- **THEN** it includes each scanned ROM address and family code so DS18B20 clients can filter family code `28`

#### Scenario: DS18B20 scan command targets parent
- **WHEN** the SPA requests a DS18B20 address scan
- **THEN** the API receives the existing `custom` command with payload `scan` for the selected OneWire parent device

#### Scenario: Non-DS18B20 scan candidate is not accepted as config
- **WHEN** a client submits an address from a non-`28` family scan result as DS18B20 config
- **THEN** the DS18B20 API adapter rejects the config even if the address came from a valid OneWire scan
