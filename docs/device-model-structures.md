# Device Model Structures

This file is the canonical reference for the device configuration and API object model used in this firmware.
The public REST endpoint contract is fixed separately in [REST API Contract](rest-api-contract.md).

Rules:
- `name`, `enabled`, and `deps` belong to the persisted config object.
- Bus identity is represented only by `deps`; current type-specific configs such as `Ssd1306DeviceConfigV6` store bus parameters and addresses, not a duplicated bus device ID.
- `config_revision` belongs to the device record wrapper, not to the type-specific config object.
- `config_version` is not part of the contract. Binary compatibility is identified by `kMagic`.
- `typeId` is an internal registry key only; the public API and frontend domain use `typeName`.
- Runtime status fields stay separate from config.

## Registered Device Types

Both `src/devices/core/DeviceTypes.cpp::DeviceTypeRegistry::withDefaults()` and
`src/integrations/common/DeviceApiAdapter.cpp::DeviceApiAdapterRegistry::withDefaults()` register
one entry per supported `typeName`, in the same order:

`dummy`, `gpio_switch`, `onewire_bus`, `i2c_bus`, `spi_bus`, `ssd1306`, `st7735`,
`ds18b20_temperature_sensor`, `ntc_thermistor_temperature_sensor`, `htu21`, `thermostat`,
`rtc_ds3231`, `pcf8574_expander`, `pcf8575_expander`, `analog_output`, `fade_analog_output`,
`scheduled_analog_output`, `analog_output_composer`, `port_expander_switch`, `schedule`,
`auto_switch`, `binary_sensor`, `dosing_pump`, `analog_port_input`, `ads1115_hub`,
`cd74hc4067_hub`, `analog_input_channel`.

`portal-spa/src/models/devices/device-model-factory.ts::allDeviceModels` mirrors the same 27 types
one-to-one (plus an `UnknownDevice` fallback for an unrecognized `typeName`, which is never
registered). This document uses DS18B20 as the running worked example throughout, since it's the
simplest complete sensor family; HTU21 and the PCF857x expanders are called out separately below
wherever they illustrate a pattern DS18B20 alone doesn't (a device with two independent readings,
and two sibling types sharing one family base). The `analog_output` family (LEDC backend plus its
`fade_analog_output`/`scheduled_analog_output`/`analog_output_composer` decorators) is a separate
worked example in [Analog Output](analog-output.md), since its `AbstractOutputDevice<ValueType>`
hierarchy and decorator dependency chain don't fit the sensor-family pattern used elsewhere in
this document. The `analog_port_input`/`ads1115_hub`/`cd74hc4067_hub`/`analog_input_channel`
family (the AnalogInput role NTC thermistor now depends on instead of owning ADC hardware itself)
is likewise a separate worked example in [Analog Input](analog-input.md), since its hub-and-channel
dependency shape mirrors the PCF857x expanders below but adds a second multi-implementor hub role
- and unlike PCF857x (two typeIds, one shared config), the channel leaf collapses to a *single*
typeId because it has no per-hub state of its own at all, not even a channel-count constant.

## Canonical Config Hierarchy

Every persisted config struct extends `DeviceBaseConfigV1` directly - there is no intermediate
`BaseSensorConfig`/`BaseOneWireSensorConfig` struct layer on the backend. Shared per-family
behavior (a `TemperatureUnit`/`TemperatureReading` pair, a `SensorFilterConfigV1` calibration+EMA
block, an `OneWireRomAddress` value type) is composed as a field or a set of free functions, not
inherited from a family base struct:

```cpp
// src/devices/core/DeviceBaseConfig.h
#pragma pack(push, 1)
struct DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "BASE-1";
    uint8_t enabled{1};
    char name[kMaxDeviceBaseNameLength + 1]{};

    DeviceValidationResult validate() const;
    bool parseJson(const JsonObjectConst& input, const char*& error);
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

// src/devices/sensors/ds18b20/Ds18b20TemperatureSensorConfig.h
#pragma pack(push, 1)
struct Ds18b20TemperatureSensorConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "DS18B20-1";
    OneWireRomAddress address{};
    uint8_t resolution{12};
    uint8_t outputUnit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{1};
    uint32_t pollMs{5000};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)
```

`deps` is deliberately not a `DeviceBaseConfigV1` field: dependency links are persisted separately
by the registry (see `docs/device-registry-persistence.md`) and only enter a config struct's JSON
shape when an adapter's `parseCreateExtras`/`parseUpdateExtras` hook copies them onto the request
(see "API Data Model" below) - `config.deps` in the public REST/bundle JSON is assembled by the
adapter layer, not stored inside the binary blob itself.

HTU21 (`src/devices/sensors/htu21/Htu21SensorConfig.h`) is the multi-reading case: its current
config also stores the I2C address (default `0x40`) and it embeds two
independent `SensorFilterConfigV1 temperatureFilter`/`humidityFilter` fields rather than a shared
sensor base, because each channel needs its own calibration/smoothing state. The PCF8574/PCF8575
expanders are the sibling-family case from the other direction: both device types share one
literal config struct, `Pcf857xExpanderConfigV2` (`src/devices/expander/PortExpanderConfig.h`,
extending `DeviceBaseConfigV1` directly, fields `i2cAddress`/`inverted`) and one codec
(`encode`/`decode`/`validate`/JSON free functions); only the runtime class and its channel count
differ between `Pcf8574ExpanderDevice` and `Pcf8575ExpanderDevice`.

`binary_sensor` (`src/devices/sensors/binary/BinarySensorDeviceConfig.h`) is the first
*input*-direction hardware seam in the codebase: it mirrors `GpioSwitchDevice`'s
`IGpioOutputDriver`/`ArduinoGpioOutputDriver` split with an `IGpioInputDriver`/
`ArduinoGpioInputDriver` pair instead, so its debounce logic is testable against a fake driver the
same way switch devices are testable against a fake output driver.

`dosing_pump` (`src/devices/dosing/DosingPumpDeviceConfig.h`) is the deepest instance yet of the
retained-state-vs-config split (see "What Is Not In Config" below and
`docs/device-registry-persistence.md`): `autoMode`, the container's current volume, today's dosed
totals, and the schedule's fired/skip-next bitmasks are all *runtime* state persisted through
`DeviceRetainedDataStore`, never part of the config blob, even though they look
configuration-shaped at a glance. It also owns state entirely outside the device registry: a
per-dose history log kept in a two-segment LittleFS ring buffer behind the `IDoseJournal` seam
(`src/devices/dosing/journal/`), independent of any single device's config revision and readable
via `GET /api/dosejournal` (see `docs/rest-api-contract.md`). Its REST adapter
(`src/integrations/rest/dosing_pump/DosingPumpDeviceApiAdapter.h`) also goes beyond
config CRUD, forwarding a small grammar of `DeviceCommandType::Custom` commands
(`startDose`/`stopDose`/`setVolume`/`skipNext`, alongside the `setMode` bridge `auto_switch`
established) through `DeviceRegistryController::cmd()`.

## Field Ownership

- `DeviceBaseConfigV1`
  - `enabled`
  - `name`
- Dependency links (`deps`)
  - persisted by the registry alongside the config blob, not inside it (see above)
  - enters public `config.deps` JSON only through the adapter's extras hooks
- `Ds18b20TemperatureSensorConfigV1`
  - `address`
  - `resolution`
  - `outputUnit`
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

`src/devices/core/DeviceTypes.h` declares the identity/record shapes as plain generic structs:

```cpp
struct DeviceRecordBase {
    DeviceId id{0};
    const char* typeName{nullptr};
    uint32_t configRevision{0};
};

struct DeviceRuntimeSnapshotBase {
    DeviceStatus status{DeviceStatus::Unknown};
    DeviceStatus effectiveStatus{DeviceStatus::Unknown};
};

template <typename TConfig, typename TRuntime> struct DeviceApiRecord {
    DeviceRecordBase record{};
    TConfig config{};
    TRuntime runtime{};
};
```

These types exist to fix the *shape* everything else must agree with, but no code actually
instantiates `DeviceApiRecord<TConfig, TRuntime>` at runtime - the real serialization path writes
JSON directly. Each device type's REST/transfer behavior is owned by one `IDeviceApiAdapter`
(`src/integrations/common/DeviceApiAdapter.h`):

```cpp
class IDeviceApiAdapter {
public:
    virtual DeviceTypeId typeId() const = 0;
    virtual const char* typeName() const = 0;
    virtual uint32_t currentConfigVersion() const = 0;

    virtual bool parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const = 0;
    virtual bool parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime,
                                          DeviceConfigUpdateRequest& request, const char*& error) const;
    virtual DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const;
    virtual DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                               const DeviceRegistry& registry) const;
    virtual DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                          const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                          uint8_t depCount, const DeviceRegistry& registry) const;

    virtual void writeDeviceJson(const IDeviceRuntime& runtime, DeviceStatus effectiveStatus, JsonObject output) const = 0;
    // Config fields only, no record/runtime - used by the device-setup transfer codec.
    virtual void writeConfigJson(const IDeviceRuntime& runtime, JsonObject config) const = 0;

protected:
    static void writeCommonDeviceJson(const IDeviceRuntime& runtime, DeviceStatus effectiveStatus, const char* typeName, JsonObject output);
};

class DeviceApiAdapterRegistry {
public:
    bool registerAdapter(const IDeviceApiAdapter& adapter);
    const IDeviceApiAdapter* find(DeviceTypeId typeId) const;
    const IDeviceApiAdapter* findByName(const char* name) const;

    static DeviceApiAdapterRegistry withDefaults();
};
```

Note that `parseCreateRequest`/`parseUpdateConfigRequest` own the *entire* create/update JSON
parse (config + deps + validation), not just a narrower "validate this already-parsed config"
step - a request never reaches `DeviceRegistry` without going through one of these first.

### Device record shape

Every adapter's `writeDeviceJson` writes the same three top-level keys - `record`, `config`,
`runtime` - via `writeCommonDeviceJson` (shared) plus its own `writeConfigJson`/`writeRuntimeJson`
overrides, so the wire shape matches `DeviceApiRecord<TConfig, TRuntime>` even though nothing
constructs that template directly.

### DS18B20 API example

`TypedDeviceApiAdapter<Derived, Device, Config>` (`src/integrations/rest/common/TypedDeviceApiAdapter.h`)
is the shared CRTP base every concrete adapter extends, so a leaf adapter only supplies its
identity/codec as static members and overrides the hooks that genuinely differ per type:

```cpp
class Ds18b20TemperatureSensorDeviceApiAdapter final
    : public TypedDeviceApiAdapter<Ds18b20TemperatureSensorDeviceApiAdapter, Ds18b20TemperatureSensorDevice,
                                   Ds18b20TemperatureSensorConfigV1> {
public:
    static constexpr const char* kTypeName = "ds18b20_temperature_sensor";
    static size_t configSize(const Ds18b20TemperatureSensorConfigV1& config);
    static bool encodeConfig(const Ds18b20TemperatureSensorConfigV1& config, uint8_t* blob, size_t capacity);

    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Ds18b20TemperatureSensorConfigV1& config,
                           DeviceCreateRequest& request, const char*& error) const;
    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Ds18b20TemperatureSensorConfigV1& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const;
    void writeRuntimeJson(const Ds18b20TemperatureSensorDevice& device, JsonObject runtimeJson) const;

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
};
```

`writeRuntimeJson` writes `output.temperature` (via the shared `writeTemperatureOutputJson`
helper), plus `consecutiveErrors` and `lastDependencyGeneration` at the top level of `runtime`.
`parseCreateExtras`/`parseUpdateExtras` parse the `deps` array (one `onewire_bus` link);
`validateCreateRequest`/`validateUpdateConfigRequest`/`validateSetDepsRequest` reject a missing
dependency or a OneWire address that collides with another device on the same bus.

HTU21's adapter (`Htu21SensorDeviceApiAdapter`) follows the identical shape, but its
`writeRuntimeJson` writes two nested objects, `output.temperature` and `output.humidity`, since
the device has two independent readings; its dependency role is `i2c_bus` instead of `onewire_bus`,
and the duplicate-dependency check is against a fixed I2C address (0x40) rather than a
user-supplied one.

### API field ownership

- `DeviceRecordBase`
  - `id`
  - `typeName`
  - `configRevision`
- `DeviceBaseConfigV1`
  - `name`
  - `enabled`
- `Ds18b20TemperatureSensorConfigV1`
  - persisted DS18B20 settings
- DS18B20 `runtime` JSON
  - `status`, `effectiveStatus` (written by `writeCommonDeviceJson`)
  - `output.temperature`
  - `consecutiveErrors`
  - `lastDependencyGeneration`

### API separation rules

- `config` contains only persisted device settings.
- `config` contains the base device fields (`name`, `enabled`) plus type-specific settings, with
  `deps` assembled alongside it by the adapter's extras hooks.
- `runtime` contains live state, status, and output data.
- `registryRevision` belongs to the response envelope, not to an individual device config object.
- The device record wrapper must not duplicate `name`, `enabled`, or `deps`; those stay inside `config`.
- `typeName` identifies the device family in the public contract; `typeId` stays internal to the registry and descriptor lookup.

## Device Setup Bundle Model

The transfer bundle used for export and import reuses the same identity/config split as the API
model, but omits runtime state - `DeviceSetupRecord<TConfig>` (`src/devices/core/DeviceTypes.h`),
serialized generically for every registered type by `DeviceSetupTransferCodec`
(`src/devices/registry/DeviceSetupTransferCodec.h`) via each type's own `IDeviceApiAdapter`
(`writeConfigJson`/`parseCreateRequest`) - adding a new device type never requires touching the
codec, only registering its adapter (see `docs/backup-and-restore.md`):

```cpp
template <typename TConfig> struct DeviceSetupRecord {
    DeviceRecordBase record{};
    TConfig config{};
};
```

### Bundle field ownership

- `record`
  - `id`
  - `typeName`
  - `configRevision`
- `config`
  - persisted device settings only
  - must include `name`, `enabled`, and `deps`
  - must not include runtime output or live status fields

### Bundle separation rules

- export should serialize `record` and `config` only
- import should accept the same `record` and `config` structure
- runtime fields must be reconstructed by the registry after import
- the bundle format should not duplicate top-level `name`, `enabled`, or `deps`

### Registry internal state

`pendingPersistence` is a backend registry-local runtime flag used to track whether the registry still needs to flush changed state. It is not part of the serialized API payload, not part of the frontend domain, and not part of the persisted device record.

## WebSocket Realtime Model

The realtime channel mirrors the same conceptual model as the API layer.
The websocket envelope remains topic-based, but device payloads use the same `record`, `config`, and `runtime` separation as the canonical device API model.

### Realtime envelope

```ts
export interface RealtimeMessage<TPayload = unknown> {
    topic: string;
    revision: number;
    payload: TPayload;
}
```

### Device realtime payload

Device upsert and command-result messages carry the shared `DeviceRecord<TConfig, TRuntime>` shape (`portal-spa/src/api/contracts.ts`).
The realtime layer does not define a separate device record type.

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
  - published with the full device payload after create, update, dependency refresh, or snapshot refresh
- `device.command_result`
  - published with the same device payload plus command-result metadata
- `device.remove`
  - published with the record identity and removal metadata, but not persisted config or runtime snapshots

### Realtime compatibility rule

The SPA normalizes websocket device messages into the same `DeviceRecord<TConfig, TRuntime>` shape it uses for API responses (`portal-spa/src/realtime/bridge.ts`, `portal-spa/src/realtime/mockSocket.ts`). Websocket payloads do not introduce a second device model.

## Contract Model Inventory

These are the models that stay aligned across backend, websocket, and frontend layers:

- `DeviceRecordBase`
  - shared identity block for backend, frontend, and websocket records (`id`, `typeName`, `configRevision`)
- `DeviceRuntimeSnapshotBase`
  - backend runtime base snapshot (`status`, `effectiveStatus`)
- `DeviceApiRecord<TConfig, TRuntime>`
  - backend canonical API payload shape (declared for reference; adapters write the equivalent JSON directly, see "API Data Model")
- `DeviceSetupRecord<TConfig>`
  - export/import bundle payload shape without runtime
- `DeviceRecord<TConfig, TRuntime>` (`portal-spa/src/api/contracts.ts`)
  - shared frontend and websocket payload
- `RealtimeMessage<TPayload>`
  - websocket transport envelope
- `DeviceCommandRequest`
  - mutation input payload
- `DeviceRegistryResponse` / `DeviceMutationResponse` / `DeviceDetailResponse`
  - frontend response wrappers around `DeviceRecord<TConfig, TRuntime>`

## Backend Adapter Pattern

Each device family owns one `IDeviceApiAdapter` instance that knows how to:

- parse a create/update request's JSON into a validated config + dependency set
- validate a create, update, or set-deps request against the live registry (duplicate names,
  dependency shape, per-family relationship rules)
- encode/decode its config to and from the binary registry blob
- write its `config`/`runtime` JSON for both the live API and (via `writeConfigJson` alone) the
  device-setup transfer bundle

`TypedDeviceApiAdapter<Derived, Device, Config>` (`src/integrations/rest/common/TypedDeviceApiAdapter.h`)
implements `parseCreateRequest`/`parseUpdateConfigRequest`/`writeDeviceJson`/`writeConfigJson`
generically in terms of `Config::parseJson`/`Config::validate`/`Config::writeJson` and the
`Device`'s `config()`/`descriptor()`, so a concrete adapter (see the DS18B20 example above) only
supplies:

- `kTypeName`, `configSize()`, `encodeConfig()` as static members
- `parseCreateExtras`/`parseUpdateExtras` if the type has dependencies or other request-only fields
- `writeConfigJson`/`writeRuntimeJson` if the default (`config.writeJson()` verbatim, empty runtime) isn't enough
- the three `validate*Request` overrides, which stay ordinary virtual overrides rather than
  CRTP-dispatched hooks

`DeviceApiAdapterRegistry::withDefaults()` (`src/integrations/common/DeviceApiAdapter.cpp`)
hand-registers one adapter instance per device type - this is the single place a new device type's
REST/transfer support gets wired in; `DeviceTypeRegistry::withDefaults()`
(`src/devices/core/DeviceTypes.cpp`) is the parallel place its runtime descriptor gets registered.

### Registry save/load

The registry never inspects a config's type-specific fields directly - it asks the owning adapter
to encode/decode the blob, and otherwise only handles the binary envelope, dependency links, and
revision bookkeeping (`docs/device-registry-persistence.md` has the full commit-order/versioning
detail).

### Rules

- The device family owns its config codec (`parseJson`/`validate`/`writeJson` on the config struct
  itself, invoked by the adapter).
- The registry owns persistence mechanics, not device-specific field parsing.
- The API layer (`IDeviceApiAdapter`) owns request/response shaping, not config internals.
- The runtime owns live state transitions, not registry serialization.
- A shared CRTP base (`TypedDeviceApiAdapter`) removes boilerplate across families, but the leaf
  adapter still owns its type-specific codec/extras/API mapping.

## Frontend Device Model

The SPA mirrors the same `record`/`config`/`runtime` split as the backend, and its domain is
camelCase end-to-end - both are true today, not aspirational.

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

### Frontend transport contract

`portal-spa/src/api/contracts.ts` is the canonical place for the shared shapes:

```ts
export type TemperatureUnit = 'celsius' | 'fahrenheit'

export interface DeviceDependencyLink {
  role: string
  deviceId: number
  invert?: boolean // Condition links only; omitted means false
}

export interface DeviceRecordBase {
  id: number
  typeName: string
  configRevision: number
}

export interface BaseDeviceConfig {
  name: string
  enabled: boolean
  deps: DeviceDependencyLink[]
}

export interface BaseDeviceRuntime {
  status: string
  lifecycleStatus: string
  effectiveStatus: string
  dependencyStatus?: string
}

export interface DeviceRecord<TConfig = BaseDeviceConfig, TRuntime = BaseDeviceRuntime> {
  record: DeviceRecordBase
  config: TConfig
  runtime: TRuntime
}

export interface DeviceCreateRequest<TConfig = BaseDeviceConfig> {
  typeName: string
  config: TConfig
}

export interface DeviceCommandRequest<TConfig = Record<string, unknown>> {
  id: number
  command: 'delete' | 'updateConfig' | 'scan' | 'setOutput' | string
  config?: Partial<TConfig>
  state?: string
}
```

There is no `type_id`, `config_version`, or `pending_persistence` anywhere in this file, and the
same is true of `portal-spa/src/stores/deviceRegistry.ts`, `deviceEventLog.ts`,
`portal-spa/src/realtime/bridge.ts`, and `mockSocket.ts` - all of them read/write the nested
`record`/`config`/`runtime` shape exclusively. `DeviceDependencyLink.role` is typed as `DeviceRole`
on both sides (the firmware enum in `src/devices/core/DeviceTypes.h`; the frontend union in
`portal-spa/src/models/device-type-ids.ts` mirrors its wire names). A device provides a small
bounded *set* of roles - most provide exactly one, a few provide two - expressed as a list on each
side:

- **Firmware**: `DeviceTypeDescriptor.providedRoles: ProvidedRoles` (`DeviceTypes.h`, a
  fixed-capacity set capped at `kMaxProvidedRoles = 3`, no heap allocation) names every role a type
  provides, if any (empty = none). Each entry is read off a role-marker interface the runtime class
  already implements -- `kProvidedRole` on `ITemperatureReadingRuntime`/`ISwitchOutputRuntime`/
  `IOneWireBusRuntime`/`II2cBusRuntime`/`ISpiBusRuntime`/`IRealTimeClockRuntime`/
  `IPortExpanderRuntime`/`IScheduleRuntime`/`IStatusRuntime`/`IAnalogInputRuntime`/
  `IAnalogInputHubRuntime` (`DeviceTypes.h`), assembled via `ProvidedRoles::of({...})` in each
  type's `descriptor()`. `IPortExpanderRuntime` is the multi-implementor case: both
  `Pcf8574ExpanderDevice` and `Pcf8575ExpanderDevice` include
  `IPortExpanderRuntime::kProvidedRole` in their `providedRoles`, so `PortExpanderSwitchDevice` (one
  channel presented as an ordinary `switch`-role device, mirroring `GpioSwitchDevice`) can depend on
  either chip family interchangeably. `IAnalogInputHubRuntime`/`DeviceRole::AnalogInputHub` is the
  same pattern applied to a second, unrelated hub family: `Ads1115HubDevice` and
  `Cd74hc4067HubDevice` both provide it, so `AnalogInputChannelDevice` (one channel, providing the
  plain `AnalogInput` role - a single typeId precisely because it never needs to know which
  concrete hub backs it) and, one level further, `NtcThermistorTemperatureSensorDevice` (a pure
  resistance-to-temperature calculator depending on `AnalogInput`) never need to know or care which
  concrete hub or leaf backs their dependency - see [Analog Input](analog-input.md).
  `IStatusRuntime`/`DeviceRole::Condition` is the
  multi-*role* case: `ScheduleDevice`, `GpioSwitchDevice`, `PortExpanderSwitchDevice`, and
  `AutoSwitchDevice` each include it *alongside* their primary role (`Schedule` or `Switch`), so a
  `Condition`-role dependency link (AutoSwitchDevice's AND-condition list) can point at any of them
  uniformly. A consumer's `DeviceDependencyRequirement` names only the role it needs (`{role,
  required}`, no type-id list); `DeviceRegistrySnapshotValidator` accepts a dependency link by
  checking `dependencyDescriptor->providedRoles.contains(requirement.role)` -- the same check for
  every role, no special-casing. This means adding a new provider of an existing role (e.g. a second
  temperature sensor type) never requires touching the consumer's descriptor.
- **Frontend**: `BaseDevice.dependencyRoles: DeviceRole[]` (`base-device.ts`, default `[]`) is the
  mirror of the same idea. `devicesForDependencyRole`/`dependencyOptionsForRole`/
  `conditionDependencyOptions` (`device-model-factory.ts`) filter the device registry by role for
  picker components (thermostat sensor/switch, bus pickers, AutoSwitch's condition list, ...) -- a
  frontend-only convenience for listing eligible devices in a dropdown, unrelated to the firmware's
  dependency-role validation and not exposed over REST.

### Frontend class hierarchy

Every concrete device extends `BaseDevice<TConfig, TCreateDraft, TOutput>`
(`portal-spa/src/models/devices/base-device.ts`) directly, *except* the two families that share
real structure across sibling types:

- `BaseDevice<TConfig, TCreateDraft, TOutput>`
  - abstract base: `typeName`, `typeId`, `dependencyRoles: DeviceRole[]`
  - abstract hooks: `createDefaultConfig()`, `createDefaultCreateDraft()`, `createEditDraft()`, `normalizeConfig()`, `normalizeOutput()`
  - shared concrete methods: `buildEditCommands()`, `buildQuickUpdateCommands()`, `buildConfigDiff()`, `buildCreatePayload()`
  - protected hooks: `encodeConfig()` (default: deep-clone), `extractCreateConfig()`, `createCreateDeps()`
- `TemperatureSensorDevice<TConfig, TCreateDraft, TOutput> extends BaseDevice<...>`
  (`temperature-sensor-device.ts`) - adds `dependencyRoles = ['temperature_sensor']`,
  `static readonly temperatureUnitOptions`, `static formatTemperature()`. Extended by
  `Ds18b20Device`, `NtcThermistorDevice`, and `Htu21Device`.
- `Pcf857xExpanderDeviceBase extends BaseDevice<...>` (`pcf857x-expander.ts`) - the sibling-family
  case: `Pcf8574ExpanderDevice`/`Pcf8575ExpanderDevice` differ only in type id/name/channel count,
  so `defaultPcf857xExpanderConfig()`/`normalizePcf857xExpanderConfig()`/`encodePcf857xExpanderConfig()`
  are hoisted to module-level functions shared by both, rather than duplicated per class.
  `Ads1115HubDevice` and `Cd74hc4067HubDevice` are *not* siblings of each other (their config
  shapes are unrelated - one is I2C-address+gain+rate, the other is four GPIO select pins plus a
  signal pin) and each extends `BaseDevice` directly.

There is no `AnalogInputChannelDeviceBase`: `AnalogInputChannelDevice` (`analog-input-channel.ts`)
is a single concrete type, not a sibling family - "one channel of whatever AnalogInputHub is wired
up" has exactly one shape regardless of which concrete hub backs it, so there is nothing for a
family base to factor out; the real channel bound comes from the selected hub at render/validation
time, not a per-type constant (see [Analog Input](analog-input.md)).

Everything else (`DummyDevice`, `GpioSwitchDevice`, `I2cBusDevice`, `SpiBusDevice`,
`OneWireBusDevice`, `Ssd1306Device`, `St7735Device`, `ThermostatDevice`, `RtcDs3231Device`,
`PortExpanderSwitchDevice`, `ScheduleDevice`, `AutoSwitchDevice`, `BinarySensorDevice`,
`DosingPumpDevice`, `AnalogPortInputDevice`, `Ads1115HubDevice`, `Cd74hc4067HubDevice`,
`AnalogInputChannelDevice`, `UnknownDevice`) extends `BaseDevice` with no family base in between.
`DosingPumpDevice` (`portal-spa/src/models/devices/dosing-pump.ts`) is the one exception to the
usual "config mirrors the wire payload" shape: its draft carries `pumpSwitchDeviceId`/
`levelSensorDeviceId`/`levelSensorInvert` as dependency *projections* alongside the real config
fields, stripped back out into `deps` links by `createCreateDeps()` and never sent as config keys
(see `protected override encodeConfig()`), the same pattern `AutoSwitchDevice` already established
for `targetSwitchDeviceId`/`conditions`.

Every concrete device follows the same static-plus-instance shape: `static defaultConfig()`,
`static normalizeConfig()`, and `static encodeConfig()` hold the type's own logic (spreading
`defaultBaseDeviceConfig()`/`normalizeBaseDeviceConfig()`/`encodeBaseDeviceConfig()` from
`base-device.ts` for the shared fields), and the required instance methods
(`createDefaultConfig`/`normalizeConfig`/`normalizeOutput`/the protected `encodeConfig`/`createCreateDeps`
overrides) just delegate to those statics. This keeps the pure data-shaping logic testable as plain
functions while still satisfying `BaseDevice`'s abstract instance-method contract.

`portal-spa/src/models/devices/device-model-factory.ts` resolves an instance by `typeName`: it
builds a flat `allDeviceModels` array (one instance per type), reduces it to a `typeId`-keyed map,
and `resolveDeviceModelByTypeName(typeName)` looks up the `typeId` via
`deviceTypeIdFromName` (`@/models/device-type-ids`) before indexing the map - falling back to
`new UnknownDevice()` for an unrecognized `typeName`.

### One device example

`Htu21Device` is a good representative example precisely because it shows both the family-base
pattern and a device with two independent readings + two independent filters:

```ts
export interface Htu21ConfigDraft extends BaseDeviceConfig {
  dependencyDeviceId: number
  unit: TemperatureUnit
  pollMs: number
  reportDeltaCelsius: number
  reportDeltaHumidity: number
  reportAlways: boolean
  temperatureFilter: SensorFilterConfig
  humidityFilter: SensorFilterConfig
}

export class Htu21Device extends TemperatureSensorDevice<Htu21ConfigDraft, Htu21CreateDraft, Htu21SensorOutputSnapshot> {
  static readonly TYPE_ID = 17 as const
  static readonly TYPE_NAME = 'htu21' as const

  readonly typeName = Htu21Device.TYPE_NAME
  readonly typeId = Htu21Device.TYPE_ID

  static defaultConfig(): Htu21ConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      dependencyDeviceId: 0,
      unit: 'celsius',
      pollMs: 5000,
      reportDeltaCelsius: 0.1,
      reportDeltaHumidity: 0.1,
      reportAlways: false,
      temperatureFilter: defaultSensorFilterConfig(),
      humidityFilter: defaultSensorFilterConfig(),
    }
  }

  static normalizeConfig(value: unknown, dependencyDeviceOrDeps?: number | DeviceDependencyLink[]): Htu21ConfigDraft {
    // Owns HTU21 config shaping, validation, and defaulting for both channels.
  }

  static encodeConfig(config: Htu21ConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      unit: config.unit,
      pollMs: config.pollMs,
      reportDeltaCelsius: config.reportDeltaCelsius,
      reportDeltaHumidity: config.reportDeltaHumidity,
      reportAlways: config.reportAlways,
      temperatureFilter: { ...config.temperatureFilter },
      humidityFilter: { ...config.humidityFilter },
    }
  }

  createDefaultConfig(): Htu21ConfigDraft {
    return Htu21Device.defaultConfig()
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Htu21ConfigDraft {
    return Htu21Device.normalizeConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): Htu21SensorOutputSnapshot {
    return record.runtime as Htu21SensorOutputSnapshot
  }

  protected override encodeConfig(config: Htu21ConfigDraft): Record<string, unknown> {
    return Htu21Device.encodeConfig(config)
  }

  protected override createCreateDeps(config: Htu21ConfigDraft): DeviceDependencyLink[] {
    return [{ role: 'i2c_bus', deviceId: config.dependencyDeviceId }]
  }
}
```

### Frontend ownership rules

- The frontend does not keep a separate snake_case domain model alongside the camelCase one.
- Forms, dialogs, tables, and dashboard widgets bind to the same camelCase config object.
- The API transport shape is a boundary concern; it is not the frontend domain model itself, but
  today the two are structurally identical (`DeviceRecord<TConfig, TRuntime>` in both).
- `configRevision` is part of the device record wrapper, not the device config object.
- `registryRevision` belongs to the collection/envelope level, not to `config`.
- `DeviceRecord` mirrors the backend `record`/`config`/`runtime` layout and must not duplicate
  `name`, `enabled`, or `deps` at the top level.
- `runtime` is read-only from the UI perspective and is never persisted as config.
- `deps` belongs to the persisted config object, not to runtime.
