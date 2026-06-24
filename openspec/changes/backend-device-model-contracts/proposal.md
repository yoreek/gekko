## Why

The backend currently exposes and persists device data through a mix of flat snapshots, nested API records, and runtime-specific ad hoc payloads. That makes the registry, API, realtime bridge, and export/import paths harder to keep consistent and forces repeated conversion logic.

## What Changes

- **BREAKING** Standardize backend device data around canonical record contracts:
  - `DeviceApiRecord<TConfig, TRuntime>` for REST/API responses
  - `DeviceRecord<TConfig, TRuntime>` for frontend/realtime consumption
  - `DeviceSetupRecord<TConfig>` for export/import bundles without runtime state
- **BREAKING** Remove flat device export/import records that duplicate `id`, `typeName`, `name`, `enabled`, `deps`, and config fields at the top level.
- **BREAKING** Keep runtime state separate from persisted config in all backend-facing contracts.
- Align registry persistence, API serialization, and realtime snapshot emission with the same identity/config split.
- Preserve device-specific config ownership inside concrete device models while keeping common base fields in the shared config layer.
- Make config revision, registry revision, and runtime status fields explicit record/runtime concerns rather than config concerns.

## Capabilities

### New Capabilities
- `device-model-contracts`: canonical backend device model contracts for API, realtime, registry, and setup bundle payloads

### Modified Capabilities
- `device-configuration`: replace flat device record interchange with canonical record/config contracts and updated field ownership
- `device-registry`: align registry persistence and reload behavior with canonical record/config/runtime boundaries
- `portal-api-controllers`: serialize and mutate device payloads through the canonical API record contract instead of flat snapshots
- `portal-realtime-state`: emit canonical realtime device payloads that match the shared record contract and bundle out runtime metadata correctly

## Impact

- Backend device registry persistence and migration code
- REST device controllers and type adapters
- WebSocket realtime message construction
- Export/import setup bundle format
- Device model classes and serializer/deserializer ownership
- Existing device-related tests and contract fixtures
