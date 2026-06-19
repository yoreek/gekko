## ADDED Requirements

### Requirement: Device UI submits structured commands
The SPA SHALL submit device commands with command-specific JSON fields instead of generic string payloads for migrated command actions.

#### Scenario: Rename sends name
- **WHEN** the user renames a device from the SPA
- **THEN** the SPA sends `command = "rename"` with `name` containing the new device name and omits `payload`

#### Scenario: Status command sends status
- **WHEN** the SPA exposes a set-status action for a device
- **THEN** it sends `command = "set_status"` with `status` containing the requested status and omits `payload`

#### Scenario: OneWire scan sends scan command
- **WHEN** the user starts a OneWire scan directly or from DS18B20 address selection
- **THEN** the SPA sends `command = "scan"` to the selected OneWire parent and omits `payload`

#### Scenario: Switch output sends state
- **WHEN** the user controls a switch-like device output
- **THEN** the SPA sends `command = "set_output"` with a `state` field and omits packed strings such as `state=on`

### Requirement: Device config edits submit JSON config
The SPA SHALL submit typed device configuration edits as JSON `config` objects and SHALL NOT encode firmware binary config blobs.

#### Scenario: DS18B20 edit sends config object
- **WHEN** the user edits a DS18B20 temperature sensor configuration
- **THEN** the SPA sends `command = "update_config"` with a JSON `config` object and omits binary `payload`

#### Scenario: OneWire edit sends config object
- **WHEN** the user edits a OneWire bus GPIO pin, internal pull-up, or enabled config state
- **THEN** the SPA sends `command = "update_config"` with `config.enabled`, `config.gpio_pin`, and `config.internal_pullup`

#### Scenario: GPIO switch edit sends config object
- **WHEN** the user edits GPIO switch configuration
- **THEN** the SPA sends `command = "update_config"` with named config fields for enabled state, GPIO pin, startup state, safe state, restore-previous-state, and inversion

#### Scenario: Frontend binary config encoders are removed
- **WHEN** device edit commands are built
- **THEN** the SPA does not call frontend helpers that construct firmware binary config blobs for OneWire or GPIO switch devices

#### Scenario: Mock mode matches production command shape
- **WHEN** the SPA runs in mock mode
- **THEN** mock command handling accepts the same structured command fields as the production API and rejects migrated legacy payload-only command shapes
