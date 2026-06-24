# Design

## Current State

The SPA currently mixes several model layers:

- `portal-spa/src/api/contracts.ts` defines a flattened `DeviceRecord`.
- `portal-spa/src/models/devices/base-device.ts` converts flattened API records into `DashboardDevice`.
- `portal-spa/src/models/device-model.ts` defines `DashboardDevice` as a view model but still carries persistence and transport fields.
- `portal-spa/src/models/devices/*.ts` use snake_case config drafts and duplicate base config fields.
- `portal-spa/src/realtime/bridge.ts` validates old websocket payloads with `device_id`, `type_id`, `config_revision`, `lifecycle_status`, and `effective_status`.
- `portal-spa/src/mock/*` stores and publishes old flattened records.

## Target Layers

### Transport Contract

`portal-spa/src/api/contracts.ts` should become the source of shared frontend contract types:

- `DeviceRecordBase`
- `DeviceRecord<TConfig, TRuntime>`
- `BaseDeviceConfig`
- `BaseDeviceRuntime`
- `DeviceRegistryResponse<TConfig, TRuntime>`
- `DeviceMutationResponse<TConfig, TRuntime>`
- `DeviceDetailResponse<TConfig, TRuntime>`
- `DeviceCreateRequest<TConfig>`
- `DeviceCommandRequest<TConfig>`

All public/frontend field names should be camelCase.

### Device Config

Every device config extends the base config:

```ts
export interface BaseDeviceConfig {
  name: string
  enabled: boolean
  deps: DeviceDependencyLink[]
}
```

Device-specific config types add only their own persisted fields.

### Device Runtime

Runtime is separate from persisted config:

```ts
export interface BaseDeviceRuntime {
  status: string
  lifecycleStatus: string
  effectiveStatus: string
  dependencyStatus?: string
}
```

Device-specific runtime types own output snapshots, live readings, scan snapshots, and command/output state.

### Device Models

`BaseDevice` should operate on:

- `device.record`
- `device.config`
- `device.runtime`

It should not read flattened transport fields such as `device_id`, `type_id`, `config_revision`, `name`, `enabled`, `deps`, or `output`.

### Dashboard View Model

`DashboardDevice` can remain as a view model for UI convenience. It must be derived from canonical records and must not become the canonical persisted model.

It should not add these fields back to device records:

- `configVersion`
- duplicated config fields outside `config`
- `pendingPersistence`

### Realtime

`RealtimeMessage<TPayload>` remains the websocket transport envelope:

```ts
export interface RealtimeMessage<TPayload = unknown> {
  topic: string
  revision: number
  payload: TPayload
}
```

Device realtime payloads should use the same `DeviceRecord<TConfig, TRuntime>` shape as API responses.

### Type Lookup

The public/frontend model uses `typeName`.

Numeric `typeId` may stay temporarily for:

- compatibility with current backend payloads
- UI component registry lookup during migration

The target registry lookup should resolve by `typeName`.

## Migration Order

1. Introduce canonical generic contract types in `api/contracts.ts`.
2. Add compatibility normalizers from current backend payloads into canonical records.
3. Move `BaseDevice` to consume canonical records.
4. Migrate device-specific model files to camelCase config/runtime types.
5. Migrate realtime bridge and mock websocket payloads to canonical records.
6. Migrate store and event log parsing.
7. Remove compatibility fields once backend API/websocket emit canonical records directly.

## Current Frontend Model Audit

- `api/contracts.ts`: flattened `DeviceRecord`; must become canonical contract root.
- `models/devices/base.ts`: draft base has `name`, `typeId`, `enabled`; should mirror base config and use `typeName`.
- `models/devices/base-device.ts`: reads flattened records; should read `record/config/runtime`.
- `models/device-model.ts`: view model layer; should stop carrying `configVersion` and `pendingPersistence`.
- `models/device-types.ts`: numeric type registries; should migrate lookup to `typeName`.
- `models/devices/dummy.ts`: empty config; should extend base config.
- `models/devices/gpio-switch.ts`: snake_case config; should define camelCase config and runtime.
- `models/devices/onewire-bus.ts`: duplicates `enabled`; should inherit base config.
- `models/devices/ds18b20.ts`: uses `dependency_device_id`; should model dependencies through `config.deps`.
- `models/devices/thermostat.ts`: uses dependency ids as config fields; should model dependencies through `config.deps`.
- `models/devices/switch.ts`: mixed shared config/output helpers; should split config from runtime/output.
- `models/devices/device-model-factory.ts`: resolves by `type_id`; should resolve by `record.typeName`.
- `stores/deviceRegistry.ts`: should not expose `pendingPersistence` from device records.
- `stores/deviceEventLog.ts`: should parse canonical realtime records.
- `realtime/bridge.ts`: should validate canonical `DeviceRecord<TConfig, TRuntime>`.
- `realtime/mockSocket.ts` and `mock/*`: should store and publish canonical records.
