## ADDED Requirements

### Requirement: OneWire edit submits JSON config
The SPA SHALL submit OneWire bus edit configuration as a JSON config object instead of constructing an opaque binary payload string.

#### Scenario: OneWire edit sends config object
- **WHEN** the user edits a OneWire bus GPIO pin, internal pull-up, or enabled config state
- **THEN** the SPA sends a device command with `command = "update_config"` and a `config` object containing `enabled`, `gpio_pin`, and `internal_pullup`

#### Scenario: OneWire edit does not send binary payload
- **WHEN** the SPA builds a OneWire bus `update_config` command
- **THEN** the command omits the binary `payload` field and does not call a frontend OneWire binary blob encoder

#### Scenario: Mock mode accepts the production command shape
- **WHEN** the SPA runs in mock mode and receives a OneWire `update_config` command with JSON config
- **THEN** the mock handler applies the config fields to the mock device record using the same field names as the production API contract

#### Scenario: Other command payloads remain unchanged
- **WHEN** the SPA requests a OneWire scan or sends another non-config custom command
- **THEN** it may continue to use the existing command-specific string payload where that command contract requires it
