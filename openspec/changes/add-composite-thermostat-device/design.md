## Context

This change assumes `replace-parent-with-deps` has already replaced parent/child relationships with stored `deps`, computed `has_deps`, and derived `dependents`. Thermostat builds on that model instead of introducing relationship storage itself.

Useful existing pieces:

- `DeviceRuntimeBase` provides `StateMachine`, lifecycle requests, runtime dirty flags, status, and dependency/dependent wiring after the deps change.
- DS18B20 already has a cooperative temperature reading model.
- `SwitchDeviceBase` already owns output state, startup/safe state, inversion, command handling, and retained-state behavior.
- REST adapters and the SPA already route typed devices through generic `/api/devices` endpoints.

The ReefDuino thermostat code is domain reference only. The parts to port are modes `OFF/COOL/HEAT`, target/min/max temperatures, check interval, sensor timeout, retry-after-error timeout, hysteresis band, debounce/min-switch interval, and safe-off paths.

## Goals / Non-Goals

**Goals:**

- Add `ThermostatDevice` as stable `type_id = 5`.
- Require exactly two deps: `temperature_sensor` and `switch`.
- Let `ThermostatDevice` cache typed runtime pointers, for example `temperatureSensor_` and `switch_`, populated from deps wiring.
- Use cooperative `tick(now)` flow with `src/core/StateMachine.h`.
- Implement hysteresis thermostat behavior first for heat/cool/off control.
- Command any compatible switch-like device through shared switch behavior, including future GPIO, I2C, or serial-backed switches.
- Derive thermostat effective status from both deps and expose enough state for REST, realtime, and SPA views.

**Non-Goals:**

- Do not implement PID/PWM thermostat control in v1.
- Do not merge sensor and switch hardware into one thermostat device config.
- Do not add thermostat-specific REST routes outside generic device registry endpoints.
- Do not change an existing device's type after creation.
- Do not reintroduce parent/child compatibility aliases in this change.

## Decisions

### Use deps roles `temperature_sensor` and `switch`

Thermostat uses the deps contract established by `replace-parent-with-deps`:

```json
{
  "deps": [
    { "role": "temperature_sensor", "device_id": 12 },
    { "role": "switch", "device_id": 21 }
  ]
}
```

The roles are semantic. Array order does not matter. Thermostat config does not duplicate these ids.

### Cache typed runtime dependencies in ThermostatDevice

The registry remains the source of truth and wires deps by role. The thermostat runtime should then cache typed pointers:

```cpp
ITemperatureReadingRuntime* temperatureSensor_{nullptr};
ISwitchOutputRuntime* switch_{nullptr};
```

The fields are runtime convenience, not persisted state. They make thermostat code direct and testable:

```text
CheckTemperature -> temperatureSensor_->latestTemperature(...)
ApplyOutput      -> switch_->requestOutput(...)
```

Alternative considered: have thermostat look up deps from the registry on every tick. That couples runtime logic to registry internals and makes tests harder.

### Add narrow runtime capabilities

Thermostat should not cast to DS18B20 or GPIO switch classes. Add optional runtime capability APIs:

- temperature-capable runtimes expose latest `TemperatureReading` and reading status
- switch-like runtimes expose supported output states, current output state, and internal output request with `now`

`Ds18b20TemperatureSensorDevice` implements the temperature capability. `SwitchDeviceBase` implements switch output capability for current and future switch subclasses.

Registry retained-state capture must observe runtime-driven switch changes, not only public REST commands.

### Thermostat config

Use fixed-point values and bounded timing fields:

```cpp
enum class ThermostatMode : uint8_t {
    Off = 0,
    Cool = 1,
    Heat = 2,
};

enum class ThermostatAlgorithm : uint8_t {
    Hysteresis = 1,
};

#pragma pack(push, 1)
struct ThermostatConfigV1 {
    static constexpr uint32_t kMagicKey = 0x5448524dUL; // THRM
    DeviceBaseConfigV1 base{};
    uint8_t mode{0};
    uint8_t algorithm{1};
    int32_t targetMilliCelsius{25000};
    int32_t minSafeMilliCelsius{0};
    int32_t maxSafeMilliCelsius{50000};
    uint16_t hysteresisCentiCelsius{50};
    uint32_t checkIntervalMs{1000};
    uint32_t sensorTimeoutMs{3000};
    uint32_t retryAfterErrorMs{30000};
    uint32_t minSwitchIntervalMs{5000};
};
#pragma pack(pop)
```

Validation should ensure safe min < target < safe max, hysteresis is non-zero and bounded, and timing values have practical lower/upper bounds.

### Hysteresis state machine

Use explicit states:

- `Idle`
- `Starting`
- `Ready`
- `CheckTemperature`
- `ApplyOutput`
- `RetryBackoff`
- `DependencyBlocked`
- `Disabled`
- `Faulted`
- `Deleting`

Control rules:

- heat mode turns switch on when `current <= target - hysteresis / 2`
- heat mode turns switch off when `current >= target + hysteresis / 2`
- cool mode turns switch on when `current >= target + hysteresis / 2`
- cool mode turns switch off when `current <= target - hysteresis / 2`
- inside the hysteresis band, preserve previous desired switch output
- off/disabled/faulted/deleting/dependency-blocked paths request safe off

`minSwitchIntervalMs` prevents ordinary chatter but does not delay safety off.

### API and UI shape

Thermostat create/update uses `deps`, not parent fields:

```json
{
  "type_id": 5,
  "name": "Heater thermostat",
  "deps": [
    { "role": "temperature_sensor", "device_id": 12 },
    { "role": "switch", "device_id": 21 }
  ],
  "config": {
    "mode": "heat",
    "target_celsius": 25.0,
    "hysteresis_celsius": 0.5,
    "min_safe_celsius": 0,
    "max_safe_celsius": 50,
    "check_interval_ms": 1000,
    "sensor_timeout_ms": 3000,
    "retry_after_error_ms": 30000,
    "min_switch_interval_ms": 5000
  }
}
```

The SPA should filter dependency selectors by capability:

- `temperature_sensor`: devices exposing valid temperature capability metadata
- `switch`: switch-like devices supporting `on` and `off`

## Risks / Trade-offs

- Thermostat can fight manual switch commands -> v1 treats enabled thermostat as controller; the next thermostat check can restore computed output.
- Sensor poll period can be slower than thermostat timeout -> make timeout explicit and validate/test stale readings.
- Internal switch output may bypass retained persistence -> add registry retained capture for runtime-driven changes.
- Hysteresis-only control is limited -> keep algorithm version field so PID/PWM can be added later.

## Migration Plan

1. Implement or depend on `replace-parent-with-deps`.
2. Add runtime capability APIs and implement them in DS18B20 and switch base.
3. Implement thermostat config codec and validation.
4. Implement thermostat runtime state machine and type descriptor.
5. Add REST adapter, websocket snapshot support, SPA models/forms/widgets, and tests.
6. Run `scripts/test.sh`; run SPA checks when frontend files change.

## Open Questions

- Should manual switch commands be rejected while an enabled thermostat depends on that switch, or should they remain accepted and be overwritten at the next thermostat check?
- Should safety output for tri-state switches use `off` or `disabled` in v1?
