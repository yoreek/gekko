## ADDED Requirements

### Requirement: OneWire bus adapter exposes config and scan state
The portal API SHALL expose OneWire bus device configuration and runtime scan state through the existing type-specific device API adapter boundary.

#### Scenario: Create request parses OneWire config
- **WHEN** a client creates a `OneWireBusDevice`
- **THEN** the type-specific adapter parses the shared name/enabled fields plus `config.gpio_pin` and `config.internal_pullup`, encodes the current OneWire config payload, and rejects malformed config shapes with the shared error envelope

#### Scenario: Device snapshot includes OneWire config
- **WHEN** the API serializes a `OneWireBusDevice` record
- **THEN** the device JSON includes `type: "onewire_bus"`, the persisted enabled/name/status fields, `config.gpio_pin`, and `config.internal_pullup`

#### Scenario: Device snapshot includes scan state
- **WHEN** the API serializes a `OneWireBusDevice` with a live runtime
- **THEN** the device JSON includes a `scan` object with `in_progress`, `ready`, `device_count`, `truncated`, `invalid_crc_seen`, and `devices` fields

#### Scenario: Scan device entries are selectable
- **WHEN** the API returns OneWire scan devices
- **THEN** each entry includes the uppercase 16-character ROM `address` and two-character uppercase `family_code` required by future child sensor create flows

### Requirement: OneWire scan uses the existing command endpoint
The portal API SHALL start OneWire scans through `POST /api/devices/:id/command` without adding a bus-specific route.

#### Scenario: Scan command is accepted
- **WHEN** a client posts `{"command":"custom","payload":"scan"}` to a ready OneWire bus device
- **THEN** the controller forwards the command to the runtime, returns the shared success envelope with registry revision and pending persistence fields, and subsequent scan progress is published through `StateChanged` device snapshots

#### Scenario: Scan command is rejected for wrong type or state
- **WHEN** a client sends the scan payload to a non-OneWire device, missing device, disabled bus, or already-scanning bus
- **THEN** the API returns the shared error envelope and does not mutate cached scan results
