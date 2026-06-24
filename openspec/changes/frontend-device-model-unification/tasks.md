# Tasks

## 1. Contract Types

- [ ] Replace the flattened `DeviceRecord` in `portal-spa/src/api/contracts.ts` with `DeviceRecordBase` and `DeviceRecord<TConfig, TRuntime>`.
- [ ] Add `BaseDeviceConfig`, `BaseDeviceRuntime`, and shared dependency/runtime snapshot types in camelCase.
- [ ] Make `DeviceRegistryResponse`, `DeviceMutationResponse`, and `DeviceDetailResponse` generic wrappers around `DeviceRecord<TConfig, TRuntime>`.
- [x] Update `DeviceCreateRequest` to use `typeName` and `config`.
- [x] Update `DeviceCommandRequest` to use camelCase commands and config payloads.

## 2. Compatibility Normalization

- [ ] Add a temporary normalizer for current backend REST payloads into canonical `DeviceRecord<TConfig, TRuntime>`.
- [ ] Add a temporary normalizer for current websocket payloads into canonical `DeviceRecord<TConfig, TRuntime>`.
- [ ] Keep compatibility logic isolated at API/realtime boundaries.
- [ ] Do not expose flattened legacy fields to device models after normalization.

## 3. Base Frontend Model

- [ ] Refactor `portal-spa/src/models/devices/base.ts` so draft/config base mirrors `BaseDeviceConfig`.
- [x] Refactor `portal-spa/src/models/devices/base-device.ts` to consume `device.record`, `device.config`, and `device.runtime`.
- [x] Remove `configVersion` from frontend domain/view models.
- [x] Remove `pendingPersistence` from the frontend device model and device records.
- [x] Resolve device models by `record.typeName`.

## 4. Device-Specific Models

- [ ] Refactor `dummy.ts` so config extends `BaseDeviceConfig`.
- [ ] Refactor `gpio-switch.ts` to camelCase config/runtime fields.
- [ ] Refactor `onewire-bus.ts` to inherit `name`, `enabled`, and `deps` from base config.
- [ ] Refactor `ds18b20.ts` to use `BaseOneWireSensorConfig`, camelCase fields, and `config.deps`.
- [ ] Refactor `thermostat.ts` to use camelCase fields and `config.deps` for sensor/switch dependencies.
- [ ] Split shared switch config from switch runtime/output helpers in `switch.ts`.

## 5. Stores And Views

- [ ] Update `deviceRegistry` store to store canonical records and derive `DashboardDevice` as a view model.
- [ ] Update `deviceEventLog` store to parse canonical realtime records and event metadata.
- [ ] Update create/edit forms to bind to `config` with camelCase fields.
- [ ] Update details/widgets to read display values from canonical config/runtime-derived view models.
- [ ] Keep UI component registry lookup by `typeName`; keep numeric `typeId` only as temporary compatibility.

## 6. Realtime And Mocks

- [ ] Update `RealtimeMessage` device payload handling to use `DeviceRecord<TConfig, TRuntime>`.
- [ ] Update `realtime/bridge.ts` validation to accept canonical records.
- [ ] Update `mockSocket.ts` to publish canonical device records.
- [ ] Update `mock/database.ts`, `mock/handlers.ts`, and `mock/snapshot.ts` to store and emit canonical records.

## 7. Verification

- [x] Run the frontend typecheck/build command used by the project.
- [ ] Run focused frontend tests for device commands, DS18B20, thermostat, and realtime/event log behavior.
- [ ] Do not run backend-only `scripts/test.sh` for this frontend change.
