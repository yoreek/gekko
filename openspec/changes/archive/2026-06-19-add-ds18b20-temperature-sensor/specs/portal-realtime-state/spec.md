## ADDED Requirements

### Requirement: DS18B20 realtime output updates
The firmware SHALL publish DS18B20 temperature output and lifecycle changes through the existing canonical device realtime topics.

#### Scenario: Changed temperature publishes device snapshot
- **WHEN** a DS18B20 runtime completes a successful poll and the temperature changed by at least its configured report delta according to its report policy
- **THEN** the firmware publishes a `device.upsert` or `device.command_result` payload containing the canonical DS18B20 snapshot with current `output.temperature`

#### Scenario: Report always publishes repeated readings
- **WHEN** a DS18B20 runtime has report-always enabled and a poll succeeds with an unchanged temperature
- **THEN** the firmware still publishes a canonical device snapshot for that completed reading

#### Scenario: Quiet unchanged reading does not publish
- **WHEN** a DS18B20 runtime has report-always disabled and a poll succeeds with a temperature change below its configured report delta
- **THEN** the firmware does not emit a realtime device update solely for that poll

#### Scenario: Missing reading publishes invalid output when state changes
- **WHEN** a DS18B20 runtime transitions from valid output to unavailable output after startup, reconfiguration, parent blocking, or read failure
- **THEN** the firmware publishes a canonical device snapshot with `output.temperature.valid = false`

#### Scenario: Parent status changes are reflected
- **WHEN** a DS18B20 parent bus is disabled, reconfigured, faulted, or returns ready
- **THEN** realtime device snapshots for affected DS18B20 children reflect the updated lifecycle or effective status without requiring a full page reload

#### Scenario: Frontend store merges DS18B20 updates
- **WHEN** the SPA receives a realtime DS18B20 device snapshot
- **THEN** it updates the device store temperature output, unit, status, parent fields, and config revision from the payload alone
