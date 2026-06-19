## Context

The project already has the main pieces needed for a child sensor runtime:

- `DeviceRuntimeBase` provides `StateMachine`, parent runtime pointers, child runtime attachment, runtime dirty flags, and lifecycle requests.
- `DeviceTypeDescriptor` already carries `compatibleParentTypes`, `canHaveChildren`, `maxChildren`, cadence flags, runtime factories, and config validators.
- `OneWireBusDevice` already owns the bus pin, supports scan results, exposes generic 16-hex ROM addresses, and declares `canHaveChildren = true`.
- The current OneWire production factory returns a static default driver, so this change must move production OneWire bus runtimes to per-bus driver ownership before multiple OneWire buses can work correctly.
- The old Gekko DS18B20 prototype used the right high-level shape: `busId`, `rom[8]`, `resolution`, `pollMs`, state-machine states for init/request/wait/read/fail, and reinit when bus, ROM, or resolution changes.

Research notes:

- The DS18B20 datasheet defines 9-12 bit Celsius measurements, unique 64-bit ROM addresses, multidrop OneWire operation, and resolution-dependent conversion time up to 750 ms at 12 bit: https://www.analog.com/media/en/technical-documentation/data-sheets/ds18b20.pdf
- DS18B20 ROM family code is `0x28`; address scans must keep the full 8-byte ROM and validate CRC before presenting a sensor candidate.
- The datasheet allows `Skip ROM` plus `Convert T` to trigger all DS18B20s on a bus, but `Read Scratchpad` via `Skip ROM` is only safe with a single slave because multiple slaves would collide.
- The Arduino OneWire library exposes reset/search/select/skip/write/read/CRC primitives, while DallasTemperature adds convenience APIs but also contains blocking wait paths and direct `delay()` usage in common flows: https://www.pjrc.com/teensy/td_libs_OneWire.html and https://github.com/milesburton/Arduino-Temperature-Control-Library

## Goals / Non-Goals

**Goals:**

- Add a `Ds18b20TemperatureSensorDevice` child device that can only be created with a compatible OneWire bus parent.
- Keep the firmware runtime cooperative: no long conversion waits, no `delay()`, no domain `millis()` calls when `tick(now)` provides time.
- Use `src/core/StateMachine.h` for DS18B20 initialization, conversion, read, backoff, reconfiguration, disable, and delete flow.
- Store bounded, versioned binary config for ROM address, resolution, poll period, output unit, and report policy.
- Prevent duplicate DS18B20 ROM addresses on the same OneWire parent.
- Keep sensor failures recoverable so a disconnected sensor can resume after wiring is restored without manual intervention.
- Serialize temperature output with value and unit so the SPA can display Celsius or Fahrenheit without guessing.
- Reinitialize the sensor when its own config changes or when the parent OneWire bus is reconfigured.
- Filter scan-based address selection to family code `28` and valid CRC candidates.
- Keep OneWire bus support sensor-agnostic and reusable for future child devices.

**Non-Goals:**

- Do not add alarm search, alarm thresholds, parasite-power strong pull-up support, overdrive mode, or EEPROM persistence for resolution in this change.
- Do not use `DallasTemperature` in the runtime path. Its code is useful as reference, but the project needs explicit non-blocking state transitions.
- Do not add a bus-level conversion scheduler in v1.
- Do not support changing an existing device's type.
- Do not redesign the generic registry command endpoint or add DS18B20-specific REST routes outside the existing device API.

## Decisions

### Device and module layout

Add sensor files under `src/devices/sensors/`:

- `temperature/TemperatureSensorTypes.h/.cpp` for shared temperature unit parsing, fixed-point conversion, and output serialization helpers.
- `temperature/TemperatureSensorDeviceBase.h/.cpp` only if it removes real duplication for output state and dirty tracking.
- `ds18b20/Ds18b20TemperatureSensorConfig.h/.cpp` for config codec and JSON parse/write helpers.
- `ds18b20/Ds18b20TemperatureSensorDevice.h/.cpp` for the concrete runtime.
- `ds18b20/Ds18b20OneWireProtocol.h/.cpp` for DS18B20 command constants, scratchpad parse, resolution config byte mapping, and testable protocol helpers.

The device type should use the next stable id after OneWire:

```cpp
constexpr DeviceTypeId kDs18b20TemperatureSensorTypeId = 4;
```

The descriptor should declare:

- `name = "Ds18b20TemperatureSensorDevice"`
- `currentConfigVersion = 1`
- `compatibleParentTypes = {3}`
- `canHaveChildren = false`
- `supportsCommands = false` unless a future explicit read command is added
- `supportsRetainedState = false`
- `ticks100ms = true`

Alternative considered: put DS18B20 beside `src/devices/bus/onewire`. That couples a concrete sensor to the bus domain. A dedicated `sensors/` area keeps the bus reusable and gives future temperature sensors a natural place to share code.

### Config shape

Use a POD config with fixed bounds:

```cpp
#pragma pack(push, 1)
struct Ds18b20TemperatureSensorConfigV1 {
    static constexpr uint32_t kMagicKey = 0x44533138UL; // DS18
    uint8_t enabled{1};
    OneWireRomAddress address{};
    uint8_t resolution{12};
    uint8_t outputUnit{0}; // 0=celsius, 1=fahrenheit
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{1};
    uint32_t pollMs{5000};
};
#pragma pack(pop)
```

Validation rules:

- address is exactly 8 bytes, formats as 16 uppercase hex chars in API/UI, has family code `0x28`, and has valid ROM CRC.
- resolution is 9, 10, 11, or 12.
- output unit is celsius or fahrenheit.
- `pollMs` is bounded, with default 5000 ms and minimum 1000 ms.
- report policy defaults to report on value change only.
- report delta is stored in centi-Celsius, defaults to `1` (`0.01 C`), and is used as the minimum change required to publish a changed value when `reportAlways` is false.

Parent id belongs to the registry relationship (`hasParent` and `parentDeviceId`), not inside the DS18B20 binary config. This keeps relationship validation centralized. DS18B20 edit should still submit parent changes atomically with config changes through the update-config API path by allowing the command payload to carry parent fields alongside the type-specific config blob.

Alternative considered: duplicate `parent_device_id` inside config. That creates two sources of truth and makes relationship validation weaker.

### Temperature output

Use fixed-point Celsius internally:

```cpp
struct TemperatureReading {
    int32_t milliCelsius{0};
    uint32_t measuredAt{0};
    bool valid{false};
};
```

The API adapter serializes the configured display unit:

```json
{
  "output": {
    "temperature": {
      "value": 23.625,
      "unit": "celsius",
      "unit_symbol": "C",
      "measured_at_ms": 123456,
      "valid": true
    }
  }
}
```

For Fahrenheit config, `value` is converted at serialization time and `unit` becomes `fahrenheit`. The raw internal value remains Celsius so config unit changes do not lose precision.

When no current reading exists after startup, reconfiguration, parent blocking, or read failures, the snapshot still includes a valid JSON shape with `value = 0`, `valid = false`, `measured_at_ms = 0`, and an error/status string such as `not_ready`, `parent_unavailable`, `not_found`, `crc_error`, or `out_of_range`. The UI must treat `valid = false` as unavailable and must not present `0` as a real temperature.

Report-on-change compares internal `milliCelsius` readings after scratchpad parsing using `reportDeltaCentiCelsius`. `reportAlways = true` marks runtime state dirty after every successful poll even if the value is unchanged.

Alternative considered: store `float` as the runtime state. Fixed-point avoids fragile float equality for change detection and is easier to test on native.

### OneWire parent access

Extend `IOneWireBusDriver` with the primitives DS18B20 needs:

- `reset()`
- `select(const OneWireRomAddress&)`
- `skip()`
- `write(uint8_t value, bool power = false)`
- `read()`
- `readBit()`

Each `OneWireBusDevice` production runtime should own its own `ArduinoOneWireBusDriver` instance. A static singleton driver is not acceptable because two configured OneWire bus devices on different pins would share and overwrite the same underlying `OneWire` object. Native tests can continue injecting fake drivers directly.

Expose a narrow child access path from `OneWireBusDevice`:

- parent is usable only when lifecycle status is `Ready`
- scan must not be active
- child transactions are short and serialized by a simple transaction guard
- the bus exposes a monotonically increasing `generation` value that changes when the bus hardware is reinitialized

DS18B20 records the parent generation after successful initialization. If the parent generation changes because the bus pin or pull-up changed, the child goes back through initialization and writes its scratchpad resolution again.

Alternative considered: let each child own a `OneWire` object on the same pin. That breaks bus ownership, makes scan and child traffic race, and bypasses the parent dependency model.

### DS18B20 state machine

The runtime uses explicit states:

- `Idle`
- `Starting`
- `PowerUpDelay`
- `ConfigureSensor`
- `RequestConversion`
- `WaitConversion`
- `ReadScratchpad`
- `Ready`
- `RetryBackoff`
- `DependencyBlocked`
- `Reconfiguring`
- `Disabled`
- `Faulted`
- `Deleting`

The flow:

1. `Starting` verifies a ready OneWire parent and a valid DS18B20 address.
2. `PowerUpDelay` waits a short startup interval, around 15-20 ms, using the provided `now` before the first scratchpad access.
3. `ConfigureSensor` reads scratchpad, preserves current TH/TL bytes, writes only the config byte when the configured resolution differs, and does not copy scratchpad to EEPROM.
4. `RequestConversion` sends `Match ROM`, the configured address, then `Convert T`.
5. `WaitConversion` waits with the App-provided `now` for the resolution-specific max conversion time: 94, 188, 375, or 750 ms.
6. `ReadScratchpad` sends `Match ROM` and `Read Scratchpad`, reads 9 bytes, validates scratchpad CRC, converts the raw value to `milliCelsius`, and marks output dirty according to report policy.
7. `Ready` waits until `pollMs` elapses before starting the next conversion.
8. `RetryBackoff` waits a bounded interval after missing parent, failed reset, invalid scratchpad, disconnected device, or out-of-range reading.

Read failures increment a consecutive error counter and keep retrying forever with backoff. After a configured threshold, for example 3 consecutive failures, the runtime can report `Faulted` to make the problem visible, but it must continue retrying and automatically return to normal output once the sensor responds again. This handles temporary wire disconnects without manual intervention.

Resolution is configured on every sensor initialization and every parent reinitialization. The runtime writes scratchpad only when needed and avoids EEPROM `Copy Scratchpad` in v1 to prevent unnecessary EEPROM wear.

### Multiple sensors on one bus

Use independent address-specific conversions in v1. Each DS18B20 child sends `Match ROM + Convert T` for its configured address, releases the bus while conversion is in progress, then uses `Match ROM + Read Scratchpad` to read only its own result.

The registry must reject duplicate DS18B20 addresses on the same OneWire parent. This check is snapshot-level rather than a single-record config validator: for create, update, and parent changes, inspect existing children of the target parent, decode DS18B20 configs, and reject another child with the same ROM address.

This is intentionally less optimized than a shared `Skip ROM + Convert T` bus scheduler. It avoids coordinating different poll periods, different resolutions, non-DS18B20 devices on the same bus, and report policies. It also preserves child-level state-machine ownership and makes tests deterministic.

A future bus-level temperature conversion coordinator can be added when there is measured need. That coordinator should live at the OneWire parent layer or a DS18B20 bus helper, not inside a single child sensor.

### Parent lifecycle behavior

The relationship orchestrator should distinguish disabled parents from other blocked parents:

- if a parent effective status is `disabled`, the child effective status is also `disabled`, and the child runtime does no sensor work.
- if a parent is missing, faulted, deleting, reconfiguring, or otherwise not ready, the child effective status is `dependency_blocked`.
- deleting a parent with children remains rejected and returns the dependent child ids.
- reconfiguring a parent runtime requests child reconfiguration so sensors reinitialize after the parent is ready.

This behavior matches the user's desired UI semantics: a manually disabled parent makes its children effectively disabled, while a broken or transitional parent is still a dependency problem.

### Scan and address selection

No DS18B20-specific scan route is added. The UI uses the selected parent OneWire bus scan result and filters candidates:

- address family code must be `28`
- CRC must be valid
- invalid CRC and non-DS18B20 families are not selectable for DS18B20 creation

Manual entry uses the same parser and validation. The API must reject create/update payloads with missing parent, incompatible parent type, malformed address, non-`28` family, invalid CRC, unsupported resolution, unsupported unit, or too-small poll period.

### Portal UI

Use existing shared device forms and Vuetify components:

- type catalog entry for `type_id = 4`
- required OneWire parent select populated from devices with type id 3
- address field with manual entry
- scan button that commands the selected OneWire parent with `custom` payload `scan`
- selectable scan results filtered to DS18B20 candidates
- resolution select: 9, 10, 11, 12 bit
- unit select: Celsius or Fahrenheit
- poll period number input
- report-always switch
- report delta input, defaulting to `0.01 C`
- detail view showing current temperature, unit, measured time, address, parent, resolution, poll policy, and current effective/lifecycle status

If no OneWire bus exists, DS18B20 create is disabled or shows a validation state requiring a bus first. The UI must not invent a local parent; the backend remains the source of truth.

Follow the global UI rules: no one-off color/font/radius/opacity overrides, use Vuetify controls, keep `portal-spa/src/styles/main.css` unchanged unless a shared structural rule is required.

## Risks / Trade-offs

- Independent conversion is less efficient with many sensors -> keep it for v1 correctness, then add a bus-level coordinator only if measured poll latency becomes a real issue.
- Parent reconfiguration can happen while a sensor is mid-conversion -> generation checks and parent transaction guard force the sensor back to initialization before reading stale scratchpad state.
- Static OneWire driver ownership would make multiple bus devices conflict -> change production runtime creation so each OneWire bus owns its own driver instance.
- Writing scratchpad resolution on every init adds bus traffic -> only write when scratchpad config differs and avoid EEPROM copy.
- Fixed scan cache can be stale -> manual/API validation still checks address shape and family, while runtime detects missing devices during reads.
- Native tests cannot verify electrical timing -> isolate protocol helpers and fake driver sequencing, then leave actual microsecond timing to the existing Arduino OneWire library.

## Migration Plan

1. Add tests and helpers for DS18B20 config parsing, ROM validation, unit conversion, scratchpad parsing, and resolution conversion time mapping.
2. Move OneWire bus production runtime creation to per-bus driver ownership, then extend the OneWire driver boundary and parent bus access without changing the existing scan API contract.
3. Implement DS18B20 runtime with fake-driver tests for success, CRC failure, missing device, recoverable fault, disabled parent, parent reconfigure, own config reconfigure, and report policy.
4. Register firmware type descriptor and REST adapter.
5. Add SPA catalog/form/detail/widget support, mocks, and i18n.
6. Run `scripts/test.sh` and SPA verification for changed frontend files.

Rollback is straightforward before archive: remove type id 4 registration and adapter/UI catalog entries, leaving existing OneWire bus behavior intact.

## Open Questions

- Should a later optimization coordinate `Skip ROM + Convert T` per bus for groups of DS18B20 sensors with compatible poll periods?
