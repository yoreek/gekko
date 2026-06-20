## MODIFIED Requirements

### Requirement: DummyDevice first implementation
The firmware SHALL include a `DummyDevice` type that exercises registry persistence, base configuration, lifecycle status, parent-child relationships, and integration events without requiring hardware.

#### Scenario: DummyDevice survives reboot
- **WHEN** a `DummyDevice` is created and the firmware restarts
- **THEN** the firmware restores the `DummyDevice` from NVS with the same device ID, name, enabled state, and configuration revision

#### Scenario: DummyDevice has no commands
- **WHEN** a caller sends a runtime command to a `DummyDevice`
- **THEN** the firmware rejects the command as unsupported by the runtime without changing Dummy state

#### Scenario: DummyDevice has no retained state
- **WHEN** the registry creates or reloads a `DummyDevice`
- **THEN** it does not load or save retained runtime state for that device type
