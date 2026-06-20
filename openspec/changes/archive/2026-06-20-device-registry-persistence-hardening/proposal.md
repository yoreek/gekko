## Why

The device registry already separates the versioned index from per-device records, but the current persistence flow still commits the index before the records. That leaves a recovery gap if persistence fails midway or power is lost after the index write and before all records are durable.

## What Changes

- Change the registry persistence commit order so per-device records are written before the index.
- Ensure a failed persistence attempt does not leave the next boot with an index that points at missing records.
- Persist the registry index as a bounded fixed-size binary record, not as a heap string/vector or unbounded materialized buffer.
- Version the registry storage format and reset the registry namespace when firmware sees an unsupported stored format so boot can continue safely after incompatible firmware upgrades.
- Use existing dirty-record IDs during flush so config-only changes write only affected device records instead of the full registry snapshot.
- Write the registry index only when the index is marked dirty by structural registry changes.
- Treat rename and other record-only changes as dirty record writes that do not rewrite the index.
- For create/delete, write records before index and treat the index as the authoritative record set on load.
- Remove the unnecessary copied device list from `GET /api/devices` by iterating registry records directly during JSON serialization.

## Capabilities

### New Capabilities
- `device-registry-persistence-hardening`: safer persistence commit ordering and failure handling for registry storage.

### Modified Capabilities
- `device-registry`: tighten the NVS-backed registry persistence contract so dirty record writes are selective, the index is stored in a bounded binary record, unsupported registry formats are reset safely on boot, and the committed index becomes visible only after required records are durable.
- `portal-api-controllers`: keep streamed device list responses incremental without materializing a copied `std::vector<DeviceRecord>`.

## Impact

- `src/devices/registry/DeviceRegistryStore.cpp`
- `src/devices/registry/DeviceRegistry.cpp`
- `src/devices/registry/DeviceRegistryPersistenceCoordinator.cpp`
- `src/portal/controllers/DeviceRegistryController.cpp`
- registry load/save tests covering partial persistence failure and recovery
- device list serialization path tests covering direct iteration
- selective flush tests proving unrelated device records are not rewritten
