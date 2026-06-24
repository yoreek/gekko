# Device Model Structures

This file is the canonical reference for the device configuration and API object model used in this firmware.

Rules:
- `name`, `enabled`, and `deps` belong to the persisted config object.
- `config_revision` belongs to the device record wrapper, not to the type-specific config object.
- `config_version` is not part of the contract. Binary compatibility is identified by `kMagic`.
- `typeId` is an internal registry key only; the public API and frontend domain use `typeName`.
- Runtime status fields stay separate from config.

## Canonical Config Hierarchy

The config model is intentionally layered so each device family can extend only the fields it owns.

```cpp
struct DeviceDependencyLink {
    DeviceDependencyRole role{DeviceDependencyRole::Unknown};
    DeviceId deviceId{0};
};

struct DeviceDeps {
    DeviceDependencyLink items[kMaxDeviceDependencies]{};
    uint8_t count{0};
};

struct BaseDeviceConfig {
    static constexpr char kMagic[] = "BASE-1";

    char name[kMaxDeviceBaseNameLength + 1]{};
    uint8_t enabled{1};
    DeviceDeps deps{};
};

struct BaseSensorConfig : public BaseDeviceConfig {
    // Shared sensor-specific fields can be added here later.
};

struct BaseOneWireSensorConfig : public BaseSensorConfig {
    OneWireRomAddress address{};
};

struct Ds18b20SensorConfig : public BaseOneWireSensorConfig {
    static constexpr char kMagic[] = "DS18B20-1";

    uint8_t resolution{12};
    uint8_t unit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{1};
    uint32_t pollMs{5000};
};
```

## Current Code Mapping

This is the same hierarchy expressed with the current firmware names:

- `BaseDeviceConfig` -> the target config shape that combines `DeviceBaseConfigV1` fields with the persisted dependency list
- `Ds18b20SensorConfig` -> `Ds18b20TemperatureSensorConfigV1`
- `BaseOneWireSensorConfig` -> the shared layer that should own the OneWire address field
- `BaseSensorConfig` -> the shared sensor layer that can stay empty until a second sensor family needs it

## DS18B20 Device Example

This is the full reference example for one concrete device family.

```cpp
#pragma once

#include "devices/bus/onewire/OneWireRomAddress.h"
#include "devices/core/DeviceBaseConfig.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kDs18b20TemperatureSensorTypeId = 4;
constexpr uint32_t kDs18b20DefaultPollMs = 5000;
constexpr uint32_t kDs18b20MinPollMs = 1000;
constexpr uint32_t kDs18b20MaxPollMs = 86400000UL;
constexpr uint16_t kDs18b20DefaultReportDeltaCentiCelsius = 1;

#pragma pack(push, 1)
struct BaseDeviceConfig {
    static constexpr char kMagic[] = "BASE-1";

    char name[kMaxDeviceBaseNameLength + 1]{};
    uint8_t enabled{1};
    DeviceDeps deps{};
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BaseSensorConfig : public BaseDeviceConfig {
    // Reserved for shared sensor config fields.
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BaseOneWireSensorConfig : public BaseSensorConfig {
    OneWireRomAddress address{};
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Ds18b20SensorConfig : public BaseOneWireSensorConfig {
    static constexpr char kMagic[] = "DS18B20-1";

    uint8_t resolution{12};
    uint8_t unit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{kDs18b20DefaultReportDeltaCentiCelsius};
    uint32_t pollMs{kDs18b20DefaultPollMs};
};
#pragma pack(pop)

constexpr size_t ds18b20SensorConfigSize(const Ds18b20SensorConfig&) {
    return sizeof(Ds18b20SensorConfig::kMagic) - 1U + sizeof(Ds18b20SensorConfig);
}

bool encodeDs18b20SensorConfig(const Ds18b20SensorConfig& config, uint8_t* blob, size_t capacity);
bool decodeDs18b20SensorConfig(const uint8_t* blob, size_t size, Ds18b20SensorConfig& config);
DeviceValidationResult validateDs18b20SensorConfig(const Ds18b20SensorConfig& config);
bool parseDs18b20SensorConfigJson(const JsonObjectConst& input, Ds18b20SensorConfig& config, const char*& error);
void writeDs18b20SensorConfigJson(const Ds18b20SensorConfig& config, JsonObject output);

} // namespace ewfm
```

## Field Ownership

- `BaseDeviceConfig`
  - `name`
  - `enabled`
  - `deps`
- `BaseSensorConfig`
  - shared sensor-only fields if needed later
- `BaseOneWireSensorConfig`
  - `address`
- `Ds18b20SensorConfig`
  - `resolution`
  - `unit`
  - `reportAlways`
  - `reportDeltaCentiCelsius`
  - `pollMs`

## What Is Not In Config

Do not place these in the type-specific config object:
- `config_revision`
- `registry_revision`
- `lifecycle_status`
- `effective_status`
- runtime output snapshots

Those fields belong to the device record, registry metadata, or runtime state, depending on where they are produced.

## API Data Model

The API shall expose device data as a record with separate `config` and `runtime` sections.

```cpp
struct DeviceApiEnvelope {
    uint32_t registryRevision{0};
    uint32_t deviceCount{0};
};

struct DeviceApiRecordBase {
    DeviceId id{0};
    const char* typeName{nullptr};
    uint32_t configRevision{0};
};

struct DeviceRuntimeSnapshotBase {
    DeviceStatus status{DeviceStatus::Unknown};
    DeviceStatus effectiveStatus{DeviceStatus::Unknown};
};

template <typename TConfig, typename TRuntime> struct DeviceApiRecord {
    DeviceApiRecordBase record{};
    TConfig config{};
    TRuntime runtime{};
};
```

### Device record shape

The record returned by the API should be `DeviceApiRecord<TConfig, TRuntime>` with a common identity block and separate `config` and `runtime` payloads.

### DS18B20 API example

For a DS18B20 device, the API model separates persisted settings from runtime state:

```cpp
struct Ds18b20DeviceRuntimeSnapshot : public DeviceRuntimeSnapshotBase {
    TemperatureReading temperature{};
    const char* outputStatus{"not_ready"};
    uint8_t consecutiveErrors{0};
    uint32_t lastDependencyGeneration{0};
};

using Ds18b20DeviceApiRecord = DeviceApiRecord<Ds18b20SensorConfig, Ds18b20DeviceRuntimeSnapshot>;
```

### API field ownership

- `DeviceApiEnvelope`
  - `registryRevision`
  - `deviceCount`
- `DeviceApiRecordBase`
  - `id`
  - `typeName`
  - `configRevision`
- `BaseDeviceConfig`
  - `name`
  - `enabled`
  - `deps`
- `Ds18b20SensorConfig`
  - persisted DS18B20 settings
- `Ds18b20DeviceRuntimeSnapshot`
  - `status`
  - `effectiveStatus`
  - `temperature`
  - `outputStatus`
  - `consecutiveErrors`
  - `lastDependencyGeneration`

### API separation rules

- `config` contains only persisted device settings.
- `config` contains the base device fields (`name`, `enabled`, `deps`) plus type-specific settings.
- `runtime` contains live state, status, and output data.
- `registryRevision` belongs to the response envelope, not to an individual device config object.
- The device record wrapper must not duplicate `name`, `enabled`, or `deps`; those stay inside `config`.
- `typeName` identifies the device family in the public contract; `typeId` stays internal to the registry and descriptor lookup.

### Registry internal state

`pendingPersistence` is a backend registry-local runtime flag used to track whether the registry still needs to flush changed state. It is not part of the serialized API payload, not part of the frontend domain, and not part of the persisted device record.

## WebSocket Realtime Model

The realtime channel should mirror the same conceptual model as the API layer.
The websocket envelope remains topic-based, but device payloads must use the same `record`, `config`, and `runtime` separation as the canonical device API model.

### Realtime envelope

```ts
export interface RealtimeMessage<TPayload = unknown> {
    topic: string;
    revision: number;
    payload: TPayload;
}
```

### Device realtime payload

Device upsert and command-result messages should carry the shared `DeviceRecord<TConfig, TRuntime>` shape.
The realtime layer must not define a separate device record type.

### Realtime field ownership

- `record`
  - `id`
  - `typeName`
  - `configRevision`
- `config`
  - persisted device settings, including `name`, `enabled`, and `deps`
- `runtime`
  - live runtime state, statuses, outputs, and snapshots
- `eventKind`
  - websocket event metadata, not part of the persisted device model
- `registryRevision`
  - websocket envelope revision, not part of the device payload
- `pendingPersistence`
  - backend registry bookkeeping only
  - do not include it in device websocket payloads; it is registry-local runtime bookkeeping only

### Realtime device topics

- `device.upsert`
  - should publish the full device payload after create, update, dependency refresh, or snapshot refresh
- `device.command_result`
  - should publish the same device payload plus command-result metadata if needed
- `device.remove`
  - should publish the record identity and removal metadata, but not persisted config or runtime snapshots

### Realtime compatibility rule

The SPA should normalize websocket device messages into the same `DeviceRecord<TConfig, TRuntime>` shape it uses for API responses. Websocket payloads should not introduce a second device model.

## Contract Model Inventory

These are the models that should stay aligned across backend, websocket, and frontend layers:

- `DeviceApiEnvelope`
  - API response envelope for registry metadata
- `DeviceApiRecordBase`
  - backend identity block inside API records
- `DeviceRuntimeSnapshotBase`
  - backend runtime base snapshot
- `DeviceRecordBase`
  - shared identity block for frontend and websocket records
- `DeviceApiRecord<TConfig, TRuntime>`
  - backend canonical API payload
- `DeviceRecord<TConfig, TRuntime>`
  - shared frontend and websocket payload
- `RealtimeMessage<TPayload>`
  - websocket transport envelope
- `DeviceCommandRequest`
  - mutation input payload
- `DeviceRegistryResponse`
  - registry collection response
- `DeviceMutationResponse`
  - create/update/delete response
- `DeviceDetailResponse`
  - single-device response

## OOP Serialization And Registry Pattern

The model should support object-oriented ownership of serialization, deserialization, validation, and registry persistence.

### Pattern

Each device family should expose one owning model class that knows how to:

- build defaults
- validate config
- encode config to the binary registry payload
- decode config from the binary registry payload
- map API input into the config object
- map config and runtime into API output
- apply config to an existing runtime object
- persist the record back into the registry

### Recommended class shape

```cpp
class DeviceModelBase {
public:
    virtual ~DeviceModelBase() = default;

    virtual DeviceTypeId typeId() const = 0;
    virtual const char* typeName() const = 0;

    virtual bool validateConfig(const DeviceApiRecordBase& record, const JsonObjectConst& configJson, const char*& error) const = 0;
    virtual bool decodeConfig(const uint8_t* blob, size_t size, BaseDeviceConfig& config) const = 0;
    virtual bool encodeConfig(const BaseDeviceConfig& config, DeviceConfigBlob& blob) const = 0;

    virtual void writeApiRecord(const DeviceApiRecordBase& record, const BaseDeviceConfig& config,
                                const DeviceRuntimeSnapshotBase* runtime, JsonObject output) const = 0;

    virtual bool applyConfigToRuntime(const BaseDeviceConfig& config, IDeviceRuntime& runtime, uint32_t now) const = 0;
};

template <typename TConfig, typename TRuntime> class TypedDeviceModel : public DeviceModelBase {
public:
    using ConfigType = TConfig;
    using RuntimeType = TRuntime;

    virtual bool decodeConfigTyped(const uint8_t* blob, size_t size, TConfig& config) const = 0;
    virtual bool encodeConfigTyped(const TConfig& config, DeviceConfigBlob& blob) const = 0;
    virtual void writeApiRecordTyped(const DeviceApiRecordBase& record, const TConfig& config,
                                     const TRuntime* runtime, JsonObject output) const = 0;
};
```

### DS18B20 model example

```cpp
class Ds18b20DeviceModel final : public TypedDeviceModel<Ds18b20SensorConfig, Ds18b20DeviceRuntimeSnapshot> {
public:
    DeviceTypeId typeId() const override {
        return kDs18b20TemperatureSensorTypeId;
    }

    const char* typeName() const override {
        return "ds18b20_temperature_sensor";
    }

    bool validateConfig(const DeviceApiRecordBase& record, const JsonObjectConst& configJson, const char*& error) const override;
    bool decodeConfigTyped(const uint8_t* blob, size_t size, Ds18b20SensorConfig& config) const override;
    bool encodeConfigTyped(const Ds18b20SensorConfig& config, DeviceConfigBlob& blob) const override;
    void writeApiRecordTyped(const DeviceApiRecordBase& record, const Ds18b20SensorConfig& config,
                             const Ds18b20DeviceRuntimeSnapshot* runtime, JsonObject output) const override;
    bool applyConfigToRuntime(const Ds18b20SensorConfig& config, IDeviceRuntime& runtime, uint32_t now) const override;
};
```

### Registry save flow

The registry should not know device-specific config details. It should only orchestrate the model.

```cpp
bool saveDeviceRecord(const DeviceModelBase& model,
                      const DeviceApiRecordBase& record,
                      const BaseDeviceConfig& config,
                      const IDeviceRuntime* runtime,
                      DeviceConfigBlob& blob) {
    if (!model.encodeConfig(config, blob)) {
        return false;
    }
    // Persist record header, blob, deps, and revisions through the registry layer.
    return true;
}
```

### Registry load flow

The registry should load the binary blob first, then hand it to the owning model.

```cpp
bool loadDeviceRecord(const DeviceModelBase& model,
                      const uint8_t* blob,
                      size_t size,
                      BaseDeviceConfig& config) {
    return model.decodeConfig(blob, size, config);
}
```

### OOP rules

- The device family owns its config codec.
- The registry owns persistence mechanics, not device-specific field parsing.
- The API layer owns request/response shaping, not config internals.
- The runtime owns live state transitions, not registry serialization.
- Shared base classes should expose reusable behavior, but the leaf model should still own its type-specific codec and API mapping.

## Frontend Mirror Model

The SPA should mirror the same hierarchy as the backend model, but the frontend domain must stay camelCase end-to-end.

### Frontend naming rules

- Use `name`, `enabled`, and `deps` inside `config`.
- Use `record.id`, `record.typeName`, and `record.configRevision` in the record wrapper.
- Use `registryRevision` in the collection/envelope layer.
- Use `lifecycleStatus`, `effectiveStatus`, and `status` in runtime.
- Use `deviceId` inside `deps` entries.
- Use `typeName` in create requests and any public device identity payload.
- Use camelCase inside `config` and `runtime` objects.
- Keep `configVersion` out of the frontend domain model.
- Keep `config` and `runtime` separate so forms, detail cards, registry tables, and API views can consume the same record shape.
- Keep registry metadata such as `registryRevision` outside the device config object.
- Keep runtime snapshots and output state outside the persisted config object.

### Frontend hierarchy

```ts
export type TemperatureUnit = 'celsius' | 'fahrenheit';

export interface DeviceDependencyLink {
    role: string;
    deviceId: number;
}

export interface OneWireScanDeviceSnapshot {
    address: string;
    familyCode: string;
}

export interface OneWireScanSnapshot {
    inProgress: boolean;
    ready: boolean;
    deviceCount: number;
    truncated: boolean;
    invalidCrcSeen: boolean;
    devices: OneWireScanDeviceSnapshot[];
}

export interface TemperatureReading {
    value: number;
    unit: TemperatureUnit;
    unitSymbol: string;
    measuredAtMs: number;
    valid: boolean;
    status?: string;
}

export interface DeviceCreateRequest<TConfig> {
    typeName: string;
    config: TConfig;
}

export interface DeviceRegistryResponse<TConfig, TRuntime> {
    registryRevision: number;
    devices: Array<DeviceRecord<TConfig, TRuntime>>;
}

export interface DeviceMutationResponse<TConfig, TRuntime> {
    registryRevision: number;
    device?: DeviceRecord<TConfig, TRuntime>;
}

export interface DeviceDetailResponse<TConfig, TRuntime> {
    registryRevision: number;
    device: DeviceRecord<TConfig, TRuntime>;
}

export interface DeviceCommandRequest<TConfig = Record<string, unknown>> {
    id: number;
    command: 'rename' | 'enable' | 'disable' | 'delete' | 'updateConfig' | 'setStatus' | 'scan' | 'setOutput';
    config?: Partial<TConfig>;
    status?: string;
    state?: string;
}

export interface BaseDeviceConfig {
    name: string;
    enabled: boolean;
    deps: DeviceDependencyLink[];
}

export interface BaseSensorConfig extends BaseDeviceConfig {
    // Reserved for future shared sensor config fields.
}

export interface BaseOneWireSensorConfig extends BaseSensorConfig {
    address: string;
}

export interface Ds18b20SensorConfig extends BaseOneWireSensorConfig {
    resolution: 9 | 10 | 11 | 12;
    unit: TemperatureUnit;
    reportAlways: boolean;
    reportDeltaCelsius: number;
    pollMs: number;
}

export interface BaseDeviceRuntime {
    status: string;
    lifecycleStatus: string;
    effectiveStatus: string;
    dependencyStatus?: string;
}

export interface BaseSensorRuntime extends BaseDeviceRuntime {
    // Shared sensor runtime fields can be added here later.
}

export interface BaseOneWireSensorRuntime extends BaseSensorRuntime {
    scan?: OneWireScanSnapshot;
}

export interface Ds18b20SensorRuntime extends BaseOneWireSensorRuntime {
    temperature?: TemperatureReading;
    outputStatus?: string;
    consecutiveErrors?: number;
    lastDependencyGeneration?: number;
}

export interface DeviceRecordBase {
    id: number;
    typeName: string;
    configRevision: number;
}

export interface DeviceRecord<TConfig, TRuntime> {
    record: DeviceRecordBase;
    config: TConfig;
    runtime: TRuntime;
}
```

### Current frontend migration targets

The current SPA still has these legacy shapes that should be migrated to the canonical model:

- `portal-spa/src/api/contracts.ts`
  - keep the public `DeviceRecord<TConfig, TRuntime>` shape canonical: `record`, `config`, `runtime`
  - remove `config_version`, `type_id`, `pending_persistence`, and duplicated `name`, `enabled`, `deps` from the public device record
  - make `DeviceRegistryResponse`, `DeviceMutationResponse`, and `DeviceDetailResponse` wrappers around `DeviceRecord<TConfig, TRuntime>` with camelCase metadata
- `portal-spa/src/models/devices/base-device.ts`
  - make `BaseDevice` operate on `record`, `config`, and `runtime` instead of flattened API fields
  - derive dashboard display state from `device.config` and `device.runtime`
- `portal-spa/src/models/device-model.ts`
  - keep `DashboardDevice` as a view model only
  - stop treating `pendingPersistence` as a device or frontend field
  - stop carrying `configVersion`
- `portal-spa/src/models/devices/*.ts`
  - rename config fields to camelCase
  - include `name`, `enabled`, and `deps` in each config type through `BaseDeviceConfig`
  - keep output/status fields in runtime types
- `portal-spa/src/realtime/*`
  - accept the same `DeviceRecord<TConfig, TRuntime>` payload as API responses
  - keep websocket-only metadata in `RealtimeMessage`, not inside device records

### Current frontend model audit

The current frontend model layer has these concrete shapes:

- `portal-spa/src/api/contracts.ts`
  - owns the transport contract today
  - currently defines a flattened `DeviceRecord`
  - should become the canonical place for `DeviceRecordBase`, `DeviceRecord<TConfig, TRuntime>`, response wrappers, command requests, common config, and common runtime types
- `portal-spa/src/models/devices/base.ts`
  - currently defines `DeviceCreateDraftBase` with `name`, `typeId`, and `enabled`
  - should be replaced by a draft/config base that mirrors `BaseDeviceConfig`
  - should use `typeName` for creation, not `typeId`
- `portal-spa/src/models/devices/base-device.ts`
  - currently converts flattened records into `DashboardDevice`
  - currently reads `record.name`, `record.enabled`, `record.deps`, `record.output`, `record.config_revision`, `record.type_id`, and `record.device_id`
  - should read `device.record`, `device.config`, and `device.runtime`
- `portal-spa/src/models/device-model.ts`
  - currently defines `DashboardDevice` and `DashboardDeviceCollection`
  - should remain a view model layer only
  - should derive display fields from canonical records without adding persistent fields back into device records
- `portal-spa/src/models/device-types.ts`
  - currently keeps numeric `typeId` registries for UI lookup
  - should switch UI lookup to `typeName`
  - numeric ids may remain only as a temporary compatibility map while backend migration is incomplete
- `portal-spa/src/models/devices/dummy.ts`
  - currently has an empty `ConfigDraft`
  - should extend the shared base config instead of storing `name` and `enabled` in a separate draft base
- `portal-spa/src/models/devices/gpio-switch.ts`
  - currently uses snake_case config fields
  - should define `GpioSwitchConfig extends BaseDeviceConfig`
  - should move output normalization to a runtime model
- `portal-spa/src/models/devices/onewire-bus.ts`
  - currently duplicates `enabled` inside the device-specific config
  - should inherit `enabled` from `BaseDeviceConfig`
  - should use camelCase fields such as `gpioPin` and `internalPullup`
- `portal-spa/src/models/devices/ds18b20.ts`
  - currently uses `dependency_device_id` as a form helper and snake_case config fields
  - should model dependencies through `config.deps`
  - should expose `Ds18b20SensorConfig extends BaseOneWireSensorConfig`
  - should expose `Ds18b20SensorRuntime extends BaseOneWireSensorRuntime`
- `portal-spa/src/models/devices/thermostat.ts`
  - currently stores dependency ids as `temperature_sensor_device_id` and `switch_device_id`
  - should model dependencies through `config.deps`
  - should expose thermostat-specific config and runtime classes with camelCase fields
- `portal-spa/src/models/devices/switch.ts`
  - currently contains shared switch config and output helpers in snake_case
  - should split shared switch config from switch runtime/output
- `portal-spa/src/models/devices/device-model-factory.ts`
  - currently resolves models by `type_id`
  - should resolve by `record.typeName`
- `portal-spa/src/stores/deviceRegistry.ts`
  - should not expose `pendingPersistence` in frontend device records
- `portal-spa/src/stores/deviceEventLog.ts`
  - currently parses old websocket payload fields
  - should parse `payload.record`, `payload.config`, and event metadata from `RealtimeMessage`
- `portal-spa/src/realtime/bridge.ts`
  - currently validates old flattened device payloads
  - should validate canonical `DeviceRecord<TConfig, TRuntime>` payloads
- `portal-spa/src/realtime/mockSocket.ts` and `portal-spa/src/mock/*`
  - currently publish and store old flattened records
  - should use canonical records so mock, API, and websocket paths exercise the same model

### Frontend class hierarchy

The SPA should use the same inheritance ladder as the backend model:

- `BaseDeviceModel<TConfig, TRuntime>`
- `BaseSensorModel<TConfig, TRuntime>`
- `BaseOneWireSensorModel<TConfig, TRuntime>`
- `Ds18b20DeviceModel`

The class responsibilities should also mirror the backend:

- `BaseDeviceModel`
  - owns the base config fields: `name`, `enabled`, `deps`
  - owns the record identity block: `record.id`, `record.typeName`, `record.configRevision`
  - validates and serializes the shared device identity layer
  - exposes common `buildCreateRequest`, `buildEditRequest`, `normalizeConfig`, and `normalizeRuntime` hooks
- `BaseSensorModel`
  - is the shared sensor extension point
  - does not duplicate device identity fields
- `BaseOneWireSensorModel`
  - owns the OneWire-specific `address` field
  - keeps dependency wiring in the model, not in the view
- `Ds18b20DeviceModel`
  - owns DS18B20-only config and runtime fields
  - contains the only DS18B20-specific serialization, validation, and runtime normalization logic

### One device example

```ts
export class Ds18b20DeviceModel extends BaseOneWireSensorModel<Ds18b20SensorConfig, Ds18b20SensorRuntime> {
    readonly typeName = 'ds18b20_temperature_sensor';

    createDefaultConfig(): Ds18b20SensorConfig {
        return {
            name: 'New Device',
            enabled: true,
            deps: [],
            address: '',
            resolution: 12,
            unit: 'celsius',
            reportAlways: false,
            reportDeltaCelsius: 0.01,
            pollMs: 5000,
        };
    }

    normalizeConfig(value: unknown): Ds18b20SensorConfig {
        // Owns DS18B20 config shaping, validation, and defaulting.
        return this.normalizeDs18b20Config(value);
    }

    normalizeRuntime(value: unknown): Ds18b20SensorRuntime {
        // Owns temperature output, status, and dependency snapshot shaping.
        return this.normalizeDs18b20Runtime(value);
    }

    buildCreateRequest(draft: Ds18b20SensorConfig): DeviceCreateRequest<Ds18b20SensorConfig> {
        return {
            typeName: this.typeName,
            config: draft,
        };
    }
}
```

### Frontend ownership rules

- The frontend should not keep a separate snake_case domain model alongside the camelCase one.
- Forms, dialogs, tables, and dashboard widgets should bind to the same camelCase config object.
- The API transport shape is a boundary concern; it should not become the frontend domain model.
- `configRevision` is part of the device record wrapper, not the device config object.
- `registryRevision` belongs to the collection/envelope level, not to `config`.
- `DeviceRecord` is a transient in-memory wrapper and must not duplicate `name`, `enabled`, or `deps`.
- `DeviceRecord` should mirror the backend `DeviceApiRecord` layout with a nested `record` identity block.
- `runtime` is read-only from the UI perspective and should never be persisted as config.
- `deps` belongs to the persisted config object, not to runtime.
