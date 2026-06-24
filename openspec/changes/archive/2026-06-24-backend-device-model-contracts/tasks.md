## 1. Canonical backend device model

- [x] 1.1 Introduce or consolidate backend device model helpers so `DeviceApiRecord`, `DeviceRecord`, and `DeviceSetupRecord` share the same `record/config/runtime` ownership rules.
- [x] 1.2 Update core device record accessors and codec helpers in `src/devices/core` and `src/devices/registry` to keep persisted config, runtime state, and identity boundaries explicit.
- [x] 1.3 Remove any remaining backend helpers that depend on flat top-level device fields instead of the nested canonical model.

## 2. REST and realtime serialization

- [x] 2.1 Update `src/portal/controllers/DeviceRegistryController.cpp` and `src/integrations/common/DeviceApiAdapter.cpp` so REST device snapshots emit the canonical nested record shape and keep dependency links inside config ownership.
- [x] 2.2 Update device-specific REST adapters in `src/integrations/rest/*` to serialize and validate nested config/runtime boundaries instead of flat payload assumptions.
- [x] 2.3 Update `src/portal/ws/PortalWebSocketMessages.cpp` and `src/devices/registry/DeviceRegistryEventReporter.cpp` so realtime payloads use the same nested device snapshot shape as REST responses.

## 3. Registry persistence and setup transfer

- [x] 3.1 Rewrite `src/devices/registry/DeviceSetupTransferCodec.cpp` so export/import uses `DeviceSetupRecord`-style `record/config` entries and omits runtime from the bundle.
- [x] 3.2 Align `src/devices/registry/DeviceRegistry.cpp`, `src/devices/registry/DeviceRegistryStore.cpp`, and related load/save code so persisted records reconstruct the canonical nested shape on read and preserve revision ownership correctly.
- [x] 3.3 Update registry migration and validation paths so legacy flat bundle assumptions are rejected or migrated intentionally, not mixed into the new contract.

## 4. Tests and verification

- [x] 4.1 Add or update backend tests for REST serialization, realtime payloads, registry persistence, and setup bundle roundtrip coverage against the new nested model contracts.
- [x] 4.2 Run the targeted backend verification suite and fix any regressions caused by the model contract migration.
