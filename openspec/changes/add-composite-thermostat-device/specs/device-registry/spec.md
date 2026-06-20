## ADDED Requirements

### Requirement: Registry captures runtime-driven retained state
The firmware SHALL capture retained runtime state changes caused by internal runtime interactions, not only by public REST commands.

#### Scenario: Thermostat changes switch output
- **WHEN** a thermostat runtime changes a switch-like dep output and the switch marks retained state dirty
- **THEN** the registry records the switch retained state through the coalesced retained-state persistence path

#### Scenario: Failed internal output does not persist retained state
- **WHEN** a runtime-driven switch output request fails
- **THEN** the registry does not persist a retained output state for that failed request

#### Scenario: Retained capture remains bounded
- **WHEN** runtime-driven output changes occur repeatedly before the flush policy is due
- **THEN** the registry coalesces retained-state persistence using the existing retained-state bounds and dirty tracking
