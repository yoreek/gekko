## 1. Device Domain Model

- [x] 1.1 Add focused `src/devices/` modules organized by domain folders for device IDs, records, types, statuses, commands, validation results, and registry constants.
- [x] 1.2 Define bounded limits for device count, display name length, type config size, retained-state size, event detail size, per-device record size, and registry index size.
- [x] 1.3 Add a `uint32_t DeviceId` generator behind an interface with deterministic native-test behavior and an ESP32 implementation based on `esp_random()`.
- [x] 1.4 Reject reserved or duplicate generated device IDs and retry generation a bounded number of times before failing creation.
- [x] 1.5 Add device record header metadata for record/header version, device ID, type ID, config version, config revision, payload length, and optional validation/check fields.
- [x] 1.6 Add persistence policy metadata for immediate, delayed, and coalesced mutations.
- [x] 1.7 Add device type descriptors/factories that declare type id, current config version, parent compatibility, child support, command support, runtime creation, retained-state support, default persistence policies, and tick cadence needs.
- [x] 1.8 Register the first `DummyDevice` descriptor without adding real OneWire, DS18B20, I2C, switch, MQTT, WebSocket, or Home Assistant implementations.

## 2. Registry Persistence

- [x] 2.1 Add a `DeviceRegistryStore` that uses `IConfigStorage` in a separate device-registry namespace from controller `DeviceConfig`.
- [x] 2.2 Implement a versioned index record containing count and `{DeviceId, type}` entries plus one bounded per-device record for each device.
- [x] 2.3 Implement registry serialization and parsing with validation for index version, record/header version, config version, index size, record size, device ID validity, type support, enabled state, config revision, and parent device ID references.
- [x] 2.4 Treat missing NVS registry data as an empty current-version registry and corrupt/unsupported index or records as a logged recovery path that does not instantiate unsafe records.
- [x] 2.5 Implement type-specific config migration hooks that parse immutable old config layouts and produce current config records through the owning device type descriptor.
- [x] 2.6 Add native Unity tests for empty load, valid load, corrupt index recovery, corrupt record recovery, unsupported schema handling, save/load round trip, rejected oversized records/indexes, and migration from old binary config fixtures.
- [x] 2.7 Add a separate retained-state store keyed by device ID with bounded record size and independent load/save/remove behavior.
- [x] 2.8 Add native Unity tests for retained-state load, missing retained state, invalid retained state, save/remove behavior, and ensuring retained-state writes do not modify device config records.

## 3. Registry Operations

- [x] 3.1 Implement `DeviceRegistry` create, list, find-by-ID, rename, update config, enable, disable, delete, and command entry points.
- [x] 3.2 Ensure every registry mutation validates before changing in-memory records or writing to NVS.
- [x] 3.3 Implement immediate persistence mutations that write required NVS index/record changes before returning success and roll back or reject when persistence fails.
- [x] 3.4 Implement delayed persistence mutations that update the in-memory registry, mark dirty index/config records, and expose pending persistence until a later flush succeeds.
- [x] 3.5 Implement coalesced retained-state mutations that keep only the latest retained value dirty until flush policy is due.
- [x] 3.6 Track dirty index, dirty config record IDs, dirty retained-state IDs, first-dirty time, and last-change time.
- [x] 3.7 Implement flush policy using debounce delay, max delay, and a forced `flushNow()` path for controlled reboot, OTA restart, factory reset, or explicit save/apply action.
- [x] 3.8 Increment device config revision and registry revision for accepted configuration or registry mutations and avoid incrementing them again when delayed persistence later succeeds.
- [x] 3.9 Keep `updatedAt` or equivalent wall-clock metadata optional and outside ordering, migration, and correctness logic.
- [x] 3.10 Add native Unity tests for duplicate display names, device ID stability across rename/update, invalid device ID rejection, duplicate generated ID retry, unsupported type rejection, max device count, config/registry revision increments, runtime status changes not incrementing config revision, and persistence failure rollback.
- [x] 3.11 Add native Unity tests for immediate persistence success/failure, delayed dirty marking, coalesced retained-state updates, debounce flush, max-delay flush, failed flush preserving dirty state, and forced flush behavior.

## 4. Runtime Lifecycle And DummyDevice

- [x] 4.1 Add App-level cadence scheduling for fast-loop, 100 ms, and 1 s or the chosen initial cadence set, computing `now` once at the application boundary.
- [x] 4.2 Add a runtime device interface with `begin`, cadence-specific `tick...(..., uint32_t now)` behavior, `requestReconfigure`, `requestDisable`, `requestDelete`, `status`, and `handleCommand` behavior.
- [x] 4.3 Implement `DummyDevice` with explicit lifecycle/status transitions for create/start, ready, simulated fault, reconfigure, disable, and delete.
- [x] 4.4 Use `StateMachine` or an equivalent explicit state-machine adapter for `DummyDevice` transitions that are not immediate.
- [x] 4.5 Add a switch-like retained-state behavior to `DummyDevice` or a focused dummy subtype so previous-state restore can be tested without real GPIO hardware.
- [x] 4.6 Restore enabled `DummyDevice` instances from persisted records during registry load and stop disabled records from doing runtime work.
- [x] 4.7 Restore retained runtime state only when the device config requests restore-from-previous behavior and valid retained state exists.
- [x] 4.8 Add native Unity tests that drive lifecycle transitions with explicit cadence ticks and verify no device handler needs `millis()` or `clock_.millis()`.
- [x] 4.9 Add native Unity tests that verify devices are invoked only for due cadences declared by their type descriptor.
- [x] 4.10 Add native Unity tests for retained-state restore, missing retained-state fallback, and retained-state write debounce/coalescing behavior.

## 5. Relationships And Dependency Status

- [x] 5.1 Implement parent/child validation using device type descriptors, including missing parent, incompatible type, self-parent, cycle, and max-child checks.
- [x] 5.2 Reject deletion of a device with children and report dependent child device IDs; allow deletion of leaf devices after runtime stop.
- [x] 5.3 Apply compatible parent reassignment atomically with immediate persistence and event emission after the mutation is accepted.
- [x] 5.4 Propagate disabled, faulted, missing, or deleting parent status to enabled children as dependency-blocked effective status without rewriting child enabled state.
- [x] 5.5 Add native Unity tests for compatible relationships, rejected invalid graphs, restrictive parent delete, leaf delete, parent reassignment, and parent status recovery.

## 6. Integration Event And Command Interfaces

- [x] 6.1 Add bounded `DeviceEvent` and `DeviceCommand` structures with event kind, registry revision, config revision where relevant, device ID, type, status, command result, and bounded detail fields.
- [x] 6.2 Add a small transport-neutral integration/event bus interface with bounded sink registration, cadence-specific ticks, and non-blocking event fanout.
- [x] 6.3 Normalize Web UI and integration commands through the same registry command entry point.
- [x] 6.4 Emit device-created, device-updated, device-deleted, status-changed, retained-state-changed, command-accepted, command-rejected, config-persisted, and persistence-pending-cleared events after the relevant state changes or persistence flushes.
- [x] 6.5 Add integration identity helpers that derive globally unique external IDs from controller identity plus device ID for adapters such as Home Assistant.
- [ ] 6.6 Add a native test integration sink that verifies event ordering, registry/config revision values, pending persistence flags, retained-state events not treated as config mutations, bounded payload handling, rejected command reporting, external ID derivation, and that unavailable sinks do not block registry operations.

## 7. Portal API And Web UI Slice

- [ ] 7.1 Add portal routes for listing dynamic devices and returning device ID, type, name, enabled state, parent device ID, config version, config revision, registry revision, optional updatedAt metadata, pending persistence state, lifecycle status, and effective dependency status.
- [ ] 7.2 Add portal routes for creating, renaming, updating, enabling, disabling, deleting, and commanding `DummyDevice` through the registry service.
- [ ] 7.3 Add minimal Web UI controls for `DummyDevice` create/list/edit/delete/enable/disable/status-command flows using device IDs as stable references.
- [ ] 7.4 Return structured validation errors from portal routes for unsupported type, invalid device ID, duplicate ID generation failure, invalid relationship, dependent delete, bounds errors, and persistence failure.
- [ ] 7.5 Expose retained-state-dependent startup behavior in API responses without embedding frequently changing retained state in the configuration payload.
- [x] 7.6 Add an explicit save/apply API path or internal service call that invokes registry `flushNow()` for pending dirty records.
- [ ] 7.7 Add host-testable route or JSON codec coverage for successful and rejected device API payloads, version/revision fields, pending persistence state, explicit save/apply behavior, and retained-state restore metadata.

## 8. App Integration And Diagnostics

- [x] 8.1 Wire the device registry into `src/core/App` startup after controller configuration load and before runtime ticks begin.
- [x] 8.2 Add registry fast-loop, 100 ms, and 1 s cadence routing to `App::tick()` using the single loop-provided timestamp.
- [x] 8.3 Ensure API-triggered registry mutations do not call `millis()` and any timestamp-dependent debounce, max-delay, or dirty flush work is handled by the next due registry cadence tick.
- [x] 8.4 Ensure retained-state flush/debounce work is handled by a registry cadence tick rather than direct wall-clock reads inside device handlers.
- [ ] 8.5 Call forced registry flush before controlled firmware restart flows that are introduced or touched by this change.
- [ ] 8.6 Add device-domain debug logging through `src/debug/Debug.h` and a build flag such as `WITH_DEVICE_REGISTRY_DEBUG`.
- [ ] 8.7 Keep device headers private under `src/` unless a narrow public API is required by tests or external library consumers.

## 9. Verification

- [x] 9.1 Run focused native Unity tests for registry, config migration, retained-state persistence, relationships, lifecycle, events, and portal JSON behavior.
- [x] 9.2 Run `scripts/test.sh` and address formatting, cppcheck, build, or test failures.
- [ ] 9.3 Review the implementation for cooperative-flow violations, unnecessary fast-cadence work across all devices, unbounded allocations in runtime paths, direct `Serial.print` logging, accidental coupling between controller `DeviceConfig` and dynamic device registry, accidental mixing of retained runtime state into config payloads, and incorrectly treating delayed persistence as already durable.
