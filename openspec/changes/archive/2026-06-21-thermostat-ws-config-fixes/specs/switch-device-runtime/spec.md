## ADDED Requirements

### Requirement: Switch output changes are snapshot-visible
The firmware SHALL mark a switch runtime dirty when its logical output state actually changes so REST and realtime snapshots expose the updated switch state.

#### Scenario: Real output change is published
- **WHEN** a switch runtime applies a new logical output state that differs from the previous state
- **THEN** the runtime marks itself dirty and the next snapshot includes the new actual output state

#### Scenario: Thermostat-driven switch change is visible
- **WHEN** a thermostat runtime drives a downstream switch to a different output state
- **THEN** the switch output transition is exposed through the same dirty and snapshot path as a direct switch command

#### Scenario: Repeated identical output does not spam updates
- **WHEN** a switch runtime receives an output request that does not change the current logical output state
- **THEN** the runtime does not emit a redundant dirty transition solely because the request was repeated
