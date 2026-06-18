## Context

The current dynamic device stack already has the boundaries needed for a bus runtime:

- `DeviceTypeDescriptor::createRuntime` creates concrete runtimes behind `IDeviceRuntime`
- `DeviceRuntimeBase` provides parent/child wiring and `StateMachine` lifecycle support
- `GpioSwitchDevice` is the nearest pattern for a pin-configured hardware device with binary config, validation, REST adapter, frontend catalog metadata, and tests
- the old `gekko/entities/bus/BusOneWireEntity.*` prototype shows a useful bus shape: one pin, explicit scan command, scan state, device count, and 16-hex ROM address formatting

Research notes:

- Espressif's `onewire_bus` docs describe 1-Wire as a single data line plus ground, with device discovery/enumeration, CRC8, and 64-bit ROM addresses containing family code, serial number, and CRC.
- Espressif recommends an external 4.7 kOhm pull-up for reliable communication, especially with multiple devices or longer cables.
- Analog Devices' 1-Wire search documentation describes the 64-bit registration number layout, with the first 8 bits as family code and an 8-bit CRC used to verify discovered ROM numbers.

## Goals / Non-Goals

**Goals:**

- Add a generic `OneWireBusDevice` dynamic runtime with stable `type_id = 3`.
- Configure the bus with one numeric bus data pin and an optional internal pull-up setting, matching the simplicity of the GPIO switch create flow without binding validation to a specific ESP32 model.
- Keep scan flow explicit, bounded, and cooperative through the existing `StateMachine` pattern.
- Return discovered OneWire ROM addresses in a stable format that future sensors can store.
- Allow future child sensor runtimes to use the bus as a parent without changing the bus contract.
- Add REST adapter and SPA create/detail/scan support so the device can be managed from the portal.

**Non-Goals:**

- Do not add DS18B20 temperature sensor support in this change.
- Do not include or instantiate `DallasTemperature`.
- Do not add parasite-power, overdrive, alarm-search, or sensor-family-specific modes in config v1.
- Do not redesign generic device command routing or add bus-specific REST routes.
- Do not add I2C bus support in this change.

## Decisions

### Device shape

Add `OneWireBusDevice` under `src/devices/bus/onewire/`.

Suggested files:

- `OneWireBusDevice.h/.cpp`
- `OneWireBusConfig.h/.cpp` if config helpers grow enough to justify a split
- `OneWireRomAddress.h/.cpp` for parse/format/CRC helpers
- `IOneWireBusDriver.h` plus Arduino and fake implementations

Config v1 should stay small and immutable after release:

```cpp
#pragma pack(push, 1)
struct OneWireBusDeviceConfigV1 {
    static constexpr uint32_t kMagicKey = 0x4F573131UL;
    uint8_t enabled{1};
    uint8_t gpioPin{4};
    uint8_t internalPullup{0};
};
#pragma pack(pop)
```

No board-specific pin validation is added now. The pin is a numeric driver input, and the runtime reports a fault if the selected driver cannot initialize it on the current hardware. For the initial implementation, the hardware expectation is a standard 1-Wire data line with an external pull-up; `internalPullup` is an optional convenience setting for short/simple wiring, not a reliability guarantee. Overdrive, parasite power, and alarm search can be added later as config v2 fields only when there is a concrete runtime need.

### Dependency boundary

Use a small project-owned driver/scanner boundary instead of exposing the third-party library through the runtime:

```cpp
struct OneWireRomAddress {
    uint8_t bytes[8]{};
};

class IOneWireBusDriver {
public:
    virtual bool begin(uint8_t pin, bool internalPullup) = 0;
    virtual void depower() = 0;
    virtual void resetSearch() = 0;
    virtual bool search(OneWireRomAddress& address) = 0;
    virtual uint8_t crc8(const uint8_t* data, size_t len) const = 0;
};
```

Production Arduino builds can wrap `paulstoffregen/OneWire`, which is already listed as an optional dependency in `platformio.ini`. Native tests use a fake driver. This keeps host tests independent from ESP32 hardware and keeps `DallasTemperature` out of the bus layer.

Alternative considered: use Espressif's ESP-IDF `onewire_bus` component directly. It has a strong RMT/UART backend, but this project currently targets Arduino through PlatformIO and already has the Arduino OneWire dependency path. A wrapper keeps the future switch to ESP-IDF possible without changing the device/API contract.

### Runtime state machine

`OneWireBusDevice` should inherit `DeviceRuntimeBase` and use explicit states similar to `DummyDevice` and switch runtimes:

- `Idle`
- `Starting`
- `Ready`
- `Scanning`
- `Reconfiguring`
- `Disabled`
- `Faulted`
- `Deleting`

`begin(now)`, `tick100ms(now)`, and command handling must use the provided `now`. Domain states must not call `millis()` or `clock_.millis()`.

The scan command is handled as `DeviceCommandType::Custom` with payload `scan`. If the bus is not `Ready` or a scan is already active, `handleCommand()` returns false so the existing registry/controller path emits a rejected command result.

### Scan flow and memory bounds

The scan result should use fixed storage, not repeated heap allocation:

```cpp
constexpr size_t kMaxOneWireScanDevices = 16;

struct OneWireScanResult {
    OneWireRomAddress devices[kMaxOneWireScanDevices]{};
    uint8_t deviceCount{0};
    bool inProgress{false};
    bool ready{false};
    bool truncated{false};
    bool invalidCandidateSeen{false};
};
```

On scan start:

- clear previous results
- `inProgress = true`
- `ready = false`
- `truncated = false`
- mark runtime state dirty so the registry publishes `DeviceEventKind::StateChanged`

On completion:

- `inProgress = false`
- `ready = true`
- retain the latest bounded result until the next scan, disable, delete, or pin reconfigure
- mark runtime state dirty so websocket clients receive a fresh device snapshot

Invalid CRC candidates are excluded from `devices[]`. The runtime should expose enough state for the API to report that an invalid candidate was seen if this is useful for debugging, but invalid addresses must not be selectable.

The `Scanning` state should perform one `search()` pass per `Tick100ms` cadence. A 1-Wire search pass contains the protocol-level reset, `SEARCH ROM` command, and the 64 ROM bit decisions with microsecond timing handled inside the OneWire driver. The external runtime cadence does not provide that microsecond timing; it only spaces bounded search passes so multiple discovered devices do not monopolize the main loop. Do not use fast-loop ticking for this bus unless later measurements prove the 100 ms cadence is too slow for the portal workflow.

### ROM address format

Add shared helpers:

- `bool formatOneWireRomAddress(const OneWireRomAddress& address, char (&out)[17])`
- `bool parseOneWireRomAddress(const char* input, OneWireRomAddress& address)`
- `bool oneWireRomCrcValid(const IOneWireBusDriver& driver, const OneWireRomAddress& address)`

The displayed address is 16 uppercase hex characters in bus byte order:

- byte `0`: family code
- bytes `1..6`: serial number
- byte `7`: CRC

The API should also expose `family_code` as two uppercase hex characters in scan result entries. Future temperature sensor work can filter DS18B20-style family codes without the bus needing to know what those codes mean.

### Registry and relationships

`OneWireBusDevice::descriptor()`:

- `typeId = 3`
- `name = "OneWireBusDevice"`
- `currentConfigVersion = 1`
- `canHaveChildren = true`
- `maxChildren = 16`
- `supportsCommands = true`
- `supportsRetainedState = false`
- `defaultPersistencePolicy = DevicePersistencePolicy::Delayed`
- `ticks100ms = true`
- `ticksFastLoop = false`

The bus does not declare compatible parents. Future sensor device descriptors will declare `compatibleParentTypes = {3}`.

### REST adapter

Add `src/integrations/rest/onewire_bus/OneWireBusDeviceApiAdapter.*` and register it in `DeviceApiAdapterRegistry::withDefaults()`.

Create/update JSON should use:

```json
{
  "type_id": 3,
  "name": "onewire",
  "enabled": true,
  "config": {
    "gpio_pin": 4,
    "internal_pullup": false
  }
}
```

Device snapshots should include:

```json
{
  "type": "onewire_bus",
  "config": {
    "gpio_pin": 4,
    "internal_pullup": false
  },
  "scan": {
    "in_progress": false,
    "ready": true,
    "device_count": 1,
    "truncated": false,
    "invalid_crc_seen": false,
    "devices": [
      {
        "address": "28FF641D6216037C",
        "family_code": "28"
      }
    ]
  }
}
```

Use the existing command endpoint:

```json
{
  "command": "custom",
  "payload": "scan"
}
```

No `/api/onewire/*` route is added.

### Portal UI

Add a OneWire device entry to the existing frontend catalog with localized English/Russian labels, local icon key, and component registry key.

The create/edit form should be minimal:

- common name/enabled fields
- GPIO pin field
- internal pull-up toggle

The detail view should show:

- current GPIO pin
- internal pull-up state
- scan status
- scan button
- result list with ROM address and family code
- empty state when scan is ready and no devices were found

Use Vuetify components and the existing shared device form/detail registry. Do not add one-off local color, typography, opacity, or radius overrides.

## Risks / Trade-offs

- Arduino `OneWire::search()` may still do more work per call than ideal -> keep scan one search pass per 100 ms tick, cap result count, and keep the driver boundary so a granular scanner can replace it.
- Hardware reliability depends on wiring and pull-up strength -> document external pull-up expectation in UI hint/test fixtures, expose internal pull-up as optional, and do not assume internal pull-up is enough.
- Type id collisions would corrupt registry behavior -> add descriptor/catalog tests asserting `type_id = 3`.
- Scan results are runtime state, not persisted configuration -> future sensors must persist the selected ROM address in their own config.
- Fixed scan capacity can truncate large buses -> expose `truncated` and keep `maxChildren`/scan capacity documented and tested.

## Migration Plan

1. Add config/ROM helper tests first so address formatting and validation are stable.
2. Add fake driver runtime tests for lifecycle, scan success, empty scan, invalid CRC, truncation, duplicate scan rejection, disable, delete, and reconfigure.
3. Add the production Arduino driver wrapper and enable the OneWire dependency for ESP32 builds only where needed.
4. Register `OneWireBusDevice` in the device type registry and REST adapter registry.
5. Add API adapter tests for create parsing and scan snapshot serialization.
6. Add portal catalog/form/detail/mock updates and focused frontend tests or smoke checks.
7. Run `scripts/test.sh`; run the frontend build if SPA files are changed during implementation.

## Open Questions

- No blocking product decision remains for v1. Power mode, overdrive, alarm search, and DS18B20 support are intentionally deferred until a concrete child sensor change.
