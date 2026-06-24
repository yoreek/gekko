## MODIFIED Requirements

### Requirement: Realtime device store updates use canonical snapshots
The SPA SHALL merge websocket `device.upsert` and `device.command_result` payloads directly into the Pinia device store when they contain a canonical device snapshot.

#### Scenario: Snapshot update patches store
- **WHEN** a realtime device update message arrives
- **THEN** the SPA normalizes the payload into the device registry store and updates the affected device entry without refetching the full registry

#### Scenario: Realtime payload matches API device record
- **WHEN** a realtime device update message carries device data
- **THEN** the payload uses the same `DeviceRecord<TConfig, TRuntime>` shape as REST device responses
- **AND** the realtime layer does not define a separate device record model

#### Scenario: Device removal remains identity-only
- **WHEN** a `device.remove` realtime message arrives
- **THEN** the SPA removes the device from the store using the device id in the payload without reloading `/api/devices`

#### Scenario: Realtime metadata stays outside device records
- **WHEN** a realtime message includes event metadata
- **THEN** event kind and message revision remain websocket metadata
- **AND** `pendingPersistence` is not copied into the device record
