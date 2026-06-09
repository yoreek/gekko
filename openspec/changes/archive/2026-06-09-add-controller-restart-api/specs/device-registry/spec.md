## MODIFIED Requirements

### Requirement: Operation-level persistence policy
The firmware SHALL support immediate, delayed, and coalesced persistence policies for registry, configuration, and retained-state mutations.

#### Scenario: Immediate mutation succeeds
- **WHEN** a mutation with immediate persistence policy is accepted
- **THEN** the firmware writes the required NVS index or record changes before returning success and reports no pending persistence for that mutation

#### Scenario: Immediate mutation fails to persist
- **WHEN** a mutation with immediate persistence policy cannot write the required NVS changes
- **THEN** the firmware rejects or rolls back the in-memory mutation and reports the operation as failed

#### Scenario: Delayed mutation is accepted
- **WHEN** a mutation with delayed persistence policy is accepted
- **THEN** the firmware updates the in-memory registry, marks the affected index or device record dirty, and reports pending persistence for the affected scope

#### Scenario: Coalesced retained-state mutation is accepted
- **WHEN** a retained-state value changes repeatedly before the flush policy is due
- **THEN** the firmware keeps only the latest retained-state value dirty for that device and avoids writing every intermediate value to NVS

#### Scenario: Dirty records are flushed
- **WHEN** the dirty flush debounce or max-delay policy is reached on an App-scheduled registry tick
- **THEN** the firmware writes dirty index, config records, and retained-state records to NVS and clears dirty flags only for writes that succeed

#### Scenario: Forced flush is requested
- **WHEN** the firmware is about to perform a controlled reboot, OTA restart, runtime-control restart API action, factory reset, or explicit save/apply action
- **THEN** the firmware attempts to flush pending dirty registry, config, and retained-state records before continuing the controlled action

#### Scenario: Runtime-control restart flush fails
- **WHEN** a runtime-control restart API request cannot flush pending registry persistence
- **THEN** the firmware rejects the restart action and keeps running without initiating reboot
