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
- **THEN** the controller writes the response incrementally to an async response stream from the registry snapshot iterator and avoids materializing a copied device vector or concatenating the full device list into a temporary `String`

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

### Requirement: Device command requests use structured fields
The portal API SHALL expose `/api/devices/:id/command` requests as structured JSON commands with command-specific fields instead of a generic public string `payload`.

#### Scenario: Rename uses name field
- **WHEN** a client renames a device
- **THEN** the request uses `command = "rename"` with a string `name` field and does not use `payload` for the new name

#### Scenario: Set status uses status field
- **WHEN** a client sets a test or debug device status
- **THEN** the request uses `command = "set_status"` with a string `status` field and does not use `payload` for the status value

#### Scenario: Set deps remains structured
- **WHEN** a client changes a device dependency relationship
- **THEN** the request uses `command = "set_deps"` with a structured `deps` array rather than dependency fields or a packed payload string

#### Scenario: Legacy command payload is rejected for migrated commands
- **WHEN** a client sends a migrated command using only `payload` instead of the required named field
- **THEN** the API rejects the request with the standard error envelope before mutating the registry or runtime

### Requirement: Typed config update commands use JSON config
The portal API SHALL accept typed device configuration updates only as human-readable JSON `config` objects and SHALL keep binary config payload encoding inside firmware adapters and registry storage.

#### Scenario: DS18B20 update accepts only JSON config
- **WHEN** a client sends `command = "update_config"` for a DS18B20 temperature sensor
- **THEN** the DS18B20 API adapter requires a JSON `config` object, parses and validates it, encodes the internal config payload, and rejects requests that provide only binary `payload`

#### Scenario: OneWire update accepts only JSON config
- **WHEN** a client sends `command = "update_config"` for a OneWire bus
- **THEN** the OneWire API adapter requires a JSON `config` object containing `enabled`, `gpio_pin`, and `internal_pullup`, parses and validates it, encodes the internal config payload, and rejects requests that provide only binary `payload`

#### Scenario: GPIO switch update accepts only JSON config
- **WHEN** a client sends `command = "update_config"` for a GPIO switch
- **THEN** the GPIO switch API adapter requires a JSON `config` object containing the editable switch config fields, parses and validates it, encodes the internal config payload, and rejects requests that provide only binary `payload`

#### Scenario: Binary storage remains internal
- **WHEN** a typed config update is accepted through JSON config
- **THEN** the registry stores the bounded versioned binary `configPayload` generated by the firmware adapter and does not require clients to construct or inspect that binary format

### Requirement: Device snapshots expose deps
The portal API SHALL expose device dependency links as `deps` plus computed `has_deps` in canonical device snapshots.

#### Scenario: Snapshot includes deps
- **WHEN** the API serializes a device with dependency links
- **THEN** the snapshot includes `deps` entries with role and device id

#### Scenario: Snapshot computes has deps
- **WHEN** the API serializes any device
- **THEN** `has_deps` is computed from the serialized `deps` array

#### Scenario: Legacy relationship fields are absent
- **WHEN** the API serializes a device after the dependency migration
- **THEN** it does not include legacy relationship fields

#### Scenario: Delete rejection reports dependents
- **WHEN** a delete request is rejected because other devices depend on the target
- **THEN** the error response reports `dependent_device_ids`

### Requirement: Known runtime actions use named commands
The portal API SHALL expose known runtime actions as named structured commands instead of public `custom` commands with packed string payloads.

#### Scenario: OneWire scan uses scan command
- **WHEN** a client requests a OneWire bus scan
- **THEN** the request uses `command = "scan"` without a generic `payload` field, and the firmware dispatches the existing cooperative scan behavior

#### Scenario: Switch output uses set output command
- **WHEN** a client commands a switch-like device output
- **THEN** the request uses `command = "set_output"` with `state = "on"`, `"off"`, or `"disabled"` and does not use a packed `state=...` payload string

#### Scenario: Dummy output uses named fields
- **WHEN** a client commands Dummy device output behavior that remains exposed through the portal API
- **THEN** the request uses a named command and explicit field values instead of `payload = "output=1"` or `payload = "output=0"`

#### Scenario: Internal command bridge is not public API
- **WHEN** the firmware converts a structured public command into an existing internal `DeviceCommand` payload string
- **THEN** that conversion remains inside the controller or adapter layer and is not required from SPA or external REST clients

### Requirement: DS18B20 device API contract
The portal API SHALL create, mutate, and serialize DS18B20 devices through the existing generic device registry endpoints.

#### Scenario: Create request includes deps and config
- **WHEN** a client creates a DS18B20 device through `POST /api/devices`
- **THEN** the request includes `type_id = 4`, common device fields, a `deps` entry with role `onewire_bus`, and a DS18B20 config object containing address, resolution, unit, poll period, report delta, and report policy

#### Scenario: Create rejects invalid dependency or address
- **WHEN** a DS18B20 create request omits a compatible OneWire dependency or contains an invalid DS18B20 address
- **THEN** the API returns the standard error envelope and does not create a partial registry record

#### Scenario: Update config can atomically update deps
- **WHEN** a client edits DS18B20 settings after creation
- **THEN** the SPA sends one `update_config` command to `POST /api/devices/:id/command` that can carry DS18B20 config and dependency fields together as structured JSON, and the API validates the combined mutation before applying it

#### Scenario: Duplicate address is rejected
- **WHEN** a DS18B20 create or update request would duplicate an existing DS18B20 address on the same OneWire dependency
- **THEN** the API returns the standard error envelope and does not change the registry

### Requirement: DS18B20 snapshot serialization
The portal API SHALL include DS18B20 persisted config, dependency identity, lifecycle status, effective status, and latest temperature output in canonical device snapshots.

#### Scenario: Snapshot includes DS18B20 config
- **WHEN** the API serializes a DS18B20 device
- **THEN** the snapshot includes type `ds18b20_temperature_sensor`, `deps`, config address, resolution, unit, poll period, report delta, and report policy

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
The portal API SHALL expose enough OneWire scan data for DS18B20 selection while keeping scan execution on the dependency OneWire bus device.

#### Scenario: OneWire scan result remains generic
- **WHEN** the API serializes a OneWire bus scan result
- **THEN** it includes each scanned ROM address and family code so DS18B20 clients can filter family code `28`

#### Scenario: DS18B20 scan command targets dependency
- **WHEN** the SPA requests a DS18B20 address scan
- **THEN** the API receives `command = "scan"` for the selected OneWire dependency device without a public payload string

#### Scenario: Non-DS18B20 scan candidate is not accepted as config
- **WHEN** a client submits an address from a non-`28` family scan result as DS18B20 config
- **THEN** the DS18B20 API adapter rejects the config even if the address came from a valid OneWire scan

### Requirement: Thermostat device API contract
The portal API SHALL create, mutate, and serialize thermostat devices through the existing generic device registry endpoints using the deps-shaped device contract.

#### Scenario: Create request includes deps and config
- **WHEN** a client creates a thermostat device through `POST /api/devices`
- **THEN** the request includes `type_id = 5`, common device fields, `temperature_sensor` and `switch` deps, and a thermostat config object containing mode, target, hysteresis, safe min/max, check interval, sensor timeout, retry timeout, and minimum switch interval

#### Scenario: Create rejects invalid dependency set
- **WHEN** a thermostat create request omits a compatible temperature sensor or switch dep
- **THEN** the API returns the standard error envelope and does not create a partial registry record

#### Scenario: Update config can atomically update deps
- **WHEN** a client edits thermostat settings after creation
- **THEN** the SPA sends one `update_config` command to `POST /api/devices/:id/command` that can carry thermostat config and dependency fields together as structured JSON, and the API validates the combined mutation before applying it

#### Scenario: Snapshot includes thermostat config and output
- **WHEN** the API serializes a thermostat device
- **THEN** the snapshot includes the thermostat config, dep links, lifecycle status, effective status, latest temperature output, desired switch output, actual switch output when available, and last check timestamp
