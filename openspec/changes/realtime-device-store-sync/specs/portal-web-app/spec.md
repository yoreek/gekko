## ADDED Requirements

### Requirement: Realtime device store updates use canonical snapshots
The SPA SHALL merge websocket `device.upsert` and `device.command_result` payloads directly into the Pinia device store when they contain a canonical device snapshot.

#### Scenario: Snapshot update patches store
- **WHEN** a realtime device update message arrives
- **THEN** the SPA normalizes the payload into the device registry store and updates the affected device entry without refetching the full registry

#### Scenario: Legacy nested payload remains tolerated during transition
- **WHEN** a realtime device update arrives in a legacy nested form
- **THEN** the SPA may still accept it during the transition period, but the canonical flat snapshot remains the preferred contract

#### Scenario: Device removal updates the store
- **WHEN** a `device.remove` realtime message arrives
- **THEN** the SPA removes the device from the store using the device id in the payload without reloading `/api/devices`
