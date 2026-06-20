## ADDED Requirements

### Requirement: Runtime capabilities support composite devices
The runtime boundary SHALL expose narrow optional capabilities for temperature readings and switch-like output control without requiring concrete class casts.

#### Scenario: Temperature sensor exposes reading capability
- **WHEN** a runtime represents a temperature sensor with a latest reading
- **THEN** it can provide the reading and reading status through the generic runtime boundary

#### Scenario: Switch runtime exposes output capability
- **WHEN** a runtime represents a switch-like device
- **THEN** it can report supported output states, current output state, and accept an internal output request with the provided `now`

#### Scenario: Thermostat rejects missing capability
- **WHEN** a thermostat dep role references a runtime that does not expose the required capability
- **THEN** runtime validation rejects the dep before thermostat control starts
