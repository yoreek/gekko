# REST API Contract

This document is the canonical public REST contract for the portal API. The
frontend model in `portal-spa/src/api/contracts.ts` is the TypeScript mirror of
this contract.

## Contract Rules

- Public JSON uses `camelCase`.
- Device records use the nested `record`, `config`, and `runtime` shape.
- Device creation uses `typeName`, not numeric `typeId`.
- Numeric device type ids are firmware/catalog internals and are not required in
  public REST payloads.
- Device commands use camelCase command names: `updateConfig` and `setOutput`.
- Persisted settings live in `config`; live state, statuses, scans, and outputs
  live in `runtime`.
- Output values remain scalar booleans or numbers. `config.enabled: false` is
  represented by runtime status fields and does not replace the output value
  with a special sentinel.
- `registryRevision` belongs to response envelopes, not to individual device
  config objects.
- Successful JSON responses include `success: true` when a body is returned.
- Errors use `{"success":false,"code":"...","error":"..."}`.
- Routes that mutate JSON use `Content-Type: application/json`.

## Common Types

```ts
type TemperatureUnit = 'celsius' | 'fahrenheit'

interface DeviceDependencyLink {
  role: string
  deviceId: number
  invert?: boolean // Condition links only; omitted means false
}

interface DeviceRecordBase {
  id: number
  typeName: string
  configRevision: number
}

interface BaseDeviceConfig {
  name: string
  enabled: boolean
  deps: DeviceDependencyLink[]
}

interface BaseDeviceRuntime {
  status: string
  lifecycleStatus: string
  effectiveStatus: string
  dependencyStatus?: string
}

interface DeviceRecord<
  TConfig extends BaseDeviceConfig = BaseDeviceConfig,
  TRuntime extends BaseDeviceRuntime = BaseDeviceRuntime,
> {
  record: DeviceRecordBase
  config: TConfig
  runtime: TRuntime
}

interface TemperatureOutputSnapshot {
  value: number
  unit: TemperatureUnit
  unitSymbol: string
  measuredAtMs: number
  valid: boolean
  status?: string
}

interface HumidityOutputSnapshot {
  value: number
  unitSymbol: string
  measuredAtMs: number
  valid: boolean
  status?: string
}
```

Status strings currently used by device runtime snapshots include `creating`,
`starting`, `ready`, `disabled`, `faulted`, `dependency_blocked`,
`reconfiguring`, `stopping`, `deleting`, and `unknown`.

## Device Types

Supported public `typeName` values:

- `dummy`
- `gpio_switch`
- `onewire_bus`
- `ds18b20_temperature_sensor`
- `ntc_thermistor_temperature_sensor`
- `htu21`
- `thermostat`
- `rtc_ds3231`
- `pcf8574_expander`
- `pcf8575_expander`
- `port_expander_switch`
- `schedule`
- `auto_switch`
- `binary_sensor`
- `dosing_pump`
- `analog_output`
- `fade_analog_output`
- `scheduled_analog_output`
- `analog_output_composer`
- `analog_port_input`
- `ads1115_hub`
- `cd74hc4067_hub`
- `analog_input_channel`

## Device Config And Runtime

### Dummy

```ts
interface DummyConfig extends BaseDeviceConfig {}
interface DummyRuntime extends BaseDeviceRuntime {}
```

### GPIO Switch

```ts
interface GpioSwitchConfig extends BaseDeviceConfig {
  restorePreviousState: boolean
  startupState: boolean
  safeState: boolean
  inverted: boolean
  gpioPin: number
}

interface GpioSwitchRuntime extends BaseDeviceRuntime {
  output?: {
    state?: boolean
    physicalLevel?: boolean
  }
}
```

### Analog Output

```ts
interface AnalogOutputConfig extends BaseDeviceConfig {
  restorePreviousState: boolean
  startupState: number // percentage, 0..100
  safeState: number // percentage, 0..100
  inverted: boolean
  pin: number
  ledcChannel: number
  frequencyHz: number
  dutyBits: number
}

interface AnalogOutputRuntime extends BaseDeviceRuntime {
  output?: {
    state?: number // percentage, 0..100
  }
}
```

The public REST boundary uses percentages. Firmware stores analog output values
internally as integer levels from `0` through `4095`.

`fade_analog_output`, `scheduled_analog_output`, and `analog_output_composer` are
decorators on top of an `analog_output`-role device rather than physical backends;
see [Analog Output](analog-output.md) for the full decorator-chain architecture.

### Fade Analog Output

```ts
interface FadeAnalogOutputConfig extends BaseDeviceConfig {
  targetDeviceId: number // deps: one `analog_output`-role dependency
  maxStep: number // percentage per step interval, 0..100
  stepIntervalMs: number // 1..60000
}

interface FadeAnalogOutputRuntime extends BaseDeviceRuntime {
  output?: {
    state?: number // percentage, 0..100
    targetState?: number // percentage, 0..100
    transitioning?: boolean
  }
}
```

### Scheduled Analog Output

```ts
interface ScheduledAnalogOutputPoint {
  deleted: boolean
  minuteOfDay: number // 0..1439
  state: number // percentage, 0..100
}

interface ScheduledAnalogOutputConfig extends BaseDeviceConfig {
  targetDeviceId: number // deps: one `analog_output`-role dependency
  points: ScheduledAnalogOutputPoint[] // up to 10, at least one non-deleted point required
}

type AnalogOutputMode = 'off' | 'manual' | 'scheduled'

interface ScheduledAnalogOutputRuntime extends BaseDeviceRuntime {
  output?: {
    state?: number // percentage, 0..100
    requestedState?: number // percentage, 0..100
    mode?: AnalogOutputMode
    timeValid?: boolean
  }
}
```

Duplicate active `minuteOfDay` values across points are rejected. Only `mode` and the
manual level are persisted; interpolated scheduled values are never persisted. `off`,
an invalid wall-clock time, or an empty effective schedule all resolve to `0`.

### Analog Output Composer

```ts
interface AnalogOutputComposerConfig extends BaseDeviceConfig {
  targetDeviceIds: number[] // deps: ordered, repeated `analog_output`-role dependencies
}

interface AnalogOutputComposerRuntime extends BaseDeviceRuntime {
  output?: {
    mode?: AnalogOutputMode
  }
}
```

The composer provides an `analog_output_group` role rather than `analog_output` — it
does not represent one scalar output itself. Its `mode` command applies to every
compatible (`scheduled_analog_output`) child dependency.

### Analog Port Input

```ts
type AdcAttenuation = '0db' | '2_5db' | '6db' | '11db'

interface AnalogPortInputConfig extends BaseDeviceConfig {
  gpioPin: number
  attenuation: AdcAttenuation
  adcSamples: number
  reportAlways: boolean
  reportDeltaMilliVolts: number
  pollMs: number
}

interface AnalogInputReadingSnapshot {
  milliVolts?: number
  rawCode?: number
  measuredAtMs?: number
  valid?: boolean
  status?: string
}

interface AnalogPortInputRuntime extends BaseDeviceRuntime {
  output?: {
    analogInput?: AnalogInputReadingSnapshot
  }
}
```

The ESP32's own ADC on `gpioPin`, no dependency. Provides the `analog_input` role. See
[Analog Input](analog-input.md).

### ADS1115 Hub

```ts
type Ads1115Gain = 'fsr6144' | 'fsr4096' | 'fsr2048' | 'fsr1024' | 'fsr0512' | 'fsr0256'
type Ads1115DataRate = '8' | '16' | '32' | '64' | '128' | '250' | '475' | '860'

interface Ads1115HubConfig extends BaseDeviceConfig {
  i2cAddress: number
  gain: Ads1115Gain
  dataRateSps: Ads1115DataRate
}

interface Ads1115HubRuntime extends BaseDeviceRuntime {}
```

`config.deps` must contain one `i2c_bus` dependency link; `i2cAddress` defaults to `0x48`.
Provides the `analog_input_hub` role and publishes no `output` of its own — see
[Analog Input](analog-input.md) for the channel arbitration protocol.

### CD74HC4067 Hub

```ts
interface Cd74hc4067HubConfig extends BaseDeviceConfig {
  selectPins: [number, number, number, number] // S0..S3
  enablePin: number // 255 = not wired
  sigPin: number
  sigAttenuation: AdcAttenuation
}

interface Cd74hc4067HubRuntime extends BaseDeviceRuntime {}
```

No dependency — owns its GPIO pins directly. Provides the `analog_input_hub` role and
publishes no `output` of its own.

### Analog Input Channel

```ts
interface AnalogInputChannelConfig extends BaseDeviceConfig {
  channel: number
  adcSamples: number
  reportAlways: boolean
  reportDeltaMilliVolts: number
  pollMs: number
}

interface AnalogInputChannelRuntime extends BaseDeviceRuntime {
  output?: {
    analogInput?: AnalogInputReadingSnapshot
  }
}
```

`config.deps` must contain one `analog_input_hub` dependency link, pointing at an `ads1115_hub`,
a `cd74hc4067_hub`, or any other `analog_input_hub`-role provider — the role is matched
generically, not by concrete type, so there is exactly one channel type rather than one per hub
chip. `channel` must be unique among every dependent of that hub, and must fit the *attached*
hub's own `channelCount` (checked dynamically against whichever hub is actually wired up, not a
static per-type bound). Provides the `analog_input` role.

### OneWire Bus

```ts
interface OneWireBusConfig extends BaseDeviceConfig {
  gpioPin: number
  internalPullup: boolean
}

interface OneWireScanDeviceSnapshot {
  address: string
  familyCode: string
}

interface OneWireScanSnapshot {
  inProgress: boolean
  ready: boolean
  deviceCount: number
  truncated: boolean
  invalidCrcSeen: boolean
  devices: OneWireScanDeviceSnapshot[]
}

interface OneWireBusRuntime extends BaseDeviceRuntime {
  scan?: OneWireScanSnapshot
}
```

### DS18B20 Temperature Sensor

```ts
interface Ds18b20Config extends BaseDeviceConfig {
  address: string
  resolution: 9 | 10 | 11 | 12
  unit: TemperatureUnit
  pollMs: number
  reportDeltaCelsius: number
  reportAlways: boolean
}

interface Ds18b20Runtime extends BaseDeviceRuntime {
  output?: {
    temperature?: TemperatureOutputSnapshot
  }
  consecutiveErrors?: number
  lastDependencyGeneration?: number
}
```

`config.deps` must contain one `onewire_bus` dependency link.

### NTC Thermistor Temperature Sensor

```ts
type NtcFormulaMode = 'beta' | 'steinhartHart'

interface NtcThermistorConfig extends BaseDeviceConfig {
  formulaMode: NtcFormulaMode
  seriesResistorOhms: number
  supplyMilliVolts: number
  nominalResistanceOhms: number // R0, used when formulaMode is "beta"
  nominalTempCelsius: number // T0, used when formulaMode is "beta"
  betaCoefficient: number // used when formulaMode is "beta"
  steinhartA: number // used when formulaMode is "steinhartHart"
  steinhartB: number
  steinhartC: number
  unit: TemperatureUnit
  pollMs: number
  reportDeltaCelsius: number
  reportAlways: boolean
  smoothingWeight: number
  calibrationFactor: number
  calibrationOffset: number
}

interface NtcThermistorRuntime extends BaseDeviceRuntime {
  output?: {
    temperature?: TemperatureOutputSnapshot
  }
}
```

The sensor is a pure resistance-to-temperature calculator; it owns none of the ADC hardware.
`config.deps` must contain exactly one dependency link whose target provides the `analog_input`
role (`analog_port_input` or `analog_input_channel` — matched generically, not by concrete type).
`Rntc = seriesResistorOhms * Vout / (supplyMilliVolts - Vout)` where `Vout` is the
dependency's latest `milliVolts` reading, then either the Beta equation or the Steinhart-Hart
equation converts `Rntc` to temperature depending on `formulaMode`. The three filter fields are the
same flattened `smoothingWeight`/`calibrationFactor`/`calibrationOffset` shape used elsewhere (see
`SensorFilterConfig` below) — applied after the curve, before the report-delta/report-always
policy. See [Analog Input](analog-input.md) for the dependency architecture and preset table.

### HTU21 Temperature+Humidity Sensor

```ts
interface SensorFilterConfig {
  smoothingWeight: number
  calibrationFactor: number
  calibrationOffset: number
}

interface Htu21Config extends BaseDeviceConfig {
  i2cAddress: number
  unit: TemperatureUnit
  pollMs: number
  reportDeltaCelsius: number
  reportDeltaHumidity: number
  reportAlways: boolean
  temperatureFilter: SensorFilterConfig
  humidityFilter: SensorFilterConfig
}

interface Htu21Runtime extends BaseDeviceRuntime {
  output?: {
    temperature?: TemperatureOutputSnapshot
    humidity?: HumidityOutputSnapshot
  }
}
```

`config.deps` must contain one `i2c_bus` dependency link. `i2cAddress`
defaults to the standard HTU21 hardware address `0x40`; compatible devices or
address translators may use another 7-bit address. Creating or updating a
device at an address already used on the same bus is rejected. Each
filter applies linear calibration (`calibrationFactor`/`calibrationOffset`)
then EMA smoothing (`smoothingWeight`) independently for its channel before
the report-delta/report-always policy is evaluated.

### Thermostat

```ts
interface ThermostatConfig extends BaseDeviceConfig {
  mode: 'off' | 'heat' | 'cool'
  algorithm: 'hysteresis'
  targetCelsius: number
  minSafeCelsius: number
  maxSafeCelsius: number
  hysteresisCelsius: number
  checkIntervalMs: number
  sensorTimeoutMs: number
  retryAfterErrorMs: number
  minSwitchIntervalMs: number
  temperatureSensorDeviceId?: number
  switchDeviceId?: number
}

interface ThermostatRuntime extends BaseDeviceRuntime {
  output?: {
    desiredSwitchState?: boolean
    actualSwitchState?: boolean
    controlStatus?: string
    lastCheckAtMs?: number
    temperature?: TemperatureOutputSnapshot
  }
}
```

`config.deps` must contain one `temperature_sensor` dependency link and one
`switch` dependency link. `temperatureSensorDeviceId` and `switchDeviceId` are
frontend convenience mirrors; `deps` is the authoritative persisted
relationship contract.

### DS3231 RTC

```ts
interface RtcDs3231Config extends BaseDeviceConfig {
  useForSystemTimeSync: boolean
  i2cAddress: number
}

interface RtcDs3231Runtime extends BaseDeviceRuntime {
  currentEpochUtc?: number
  lastReadOk?: boolean
  oscillatorStopped?: boolean
}
```

`config.deps` must contain one `i2c_bus` dependency link. `i2cAddress`
defaults to `0x68` (the conventional DS3231 address) but is user-editable —
some breakout boards/clones answer at a different address — and is
discovered the same way as any other I2C device: scan the `i2c_bus`
dependency (`POST /api/devices/:id/command` with `{"command":"scan"}`) and
read back candidate addresses from its `runtime.scan.devices`. At most one
`rtc_ds3231` device registry-wide may have `useForSystemTimeSync: true` —
creating or updating a second one with the flag set is rejected with
`INVALID_CONFIG`. `oscillatorStopped` reflects the DS3231's oscillator-stop
(OSF) flag, which the chip sets when it lost power/was never set — a
persistent `true` after a fresh write-back means the RTC's battery is likely
dead or missing. See `GET /api/system/time`'s `source: "rtc"` for how the
active RTC device feeds system time (`RtcSyncCoordinator`,
`src/time/RtcSyncCoordinator.h`).

### PCF8574 / PCF8575 Port Expander

```ts
interface PortExpanderConfig extends BaseDeviceConfig {
  i2cAddress: number
  inverted: boolean
}

interface PortExpanderRuntime extends BaseDeviceRuntime {
  channelCount?: number
  channelStates?: number
  diagnostics?: {
    status: 'ok' | 'degraded'
    consecutiveErrors: number
    lastErrorCode: number
    lastErrorAtMs: number
    errorOps: number
  }
}
```

`pcf8574_expander` (8 channels) and `pcf8575_expander` (16 channels) are
separate device types for the same chip family — pick the one matching your
hardware. `config.deps` must contain one `i2c_bus` dependency link.
`i2cAddress` defaults to `0x20` (all address pins low) but is user-editable,
discovered the same way as any other I2C device via the `i2c_bus`
dependency's scan command. `inverted` flips every channel's electrical level
before writing the register (for boards wired active-low, e.g. many relay
modules) — this is independent of each `port_expander_switch` channel's own
`inverted` flag, which only affects that one channel's logical sense.

These chips are treated as write-only: there is no meaningful register
read-back, so `channelStates` reflects firmware's in-memory bitmask, which is
re-pushed to the chip immediately on every channel change and additionally,
unconditionally, every 10 seconds as a safety net — some PCF857x parts are
known to occasionally reset (e.g. on a brownout) and silently lose their
output state.

Neither expander type is directly usable as a switch — individual channels
are exposed via one or more `port_expander_switch` devices depending on it
(see below), the same way GPIO pins are exposed via `gpio_switch`.

### Port Expander Switch

```ts
interface PortExpanderSwitchConfig extends BaseDeviceConfig {
  restorePreviousState: boolean
  startupState: boolean
  safeState: boolean
  inverted: boolean
  channel: number
}

interface PortExpanderSwitchRuntime extends BaseDeviceRuntime {
  output?: {
    state?: boolean
    physicalLevel?: boolean
  }
}
```

A single channel of a PCF8574 or PCF8575 port expander, presented as an
ordinary on/off switch — functionally identical to `gpio_switch`
and usable anywhere a `switch` dependency is expected (e.g. Thermostat).
`config.deps` must contain one `port_expander` dependency link, which may
point at either a `pcf8574_expander` or a `pcf8575_expander` device: this
type is agnostic to which expander chip provides its channel. `channel` must
be within the dependency's actual channel count (0-7 for PCF8574, 0-15 for
PCF8575) and must not collide with another `port_expander_switch` already
using the same channel on the same expander.

### Schedule

```ts
type ScheduleRuleMode = 'alwaysOn' | 'interval'

interface ScheduleRuleConfig {
  enabled: boolean
  weekDays: number[]
  startMinuteOfDay: number
  endMinuteOfDay: number
  mode: ScheduleRuleMode
  intervalsPerWindow: number
  durationMinutes: number
}

interface ScheduleConfig extends BaseDeviceConfig {
  rules: ScheduleRuleConfig[]
}

interface ScheduleRuntime extends BaseDeviceRuntime {}
```

Up to 4 rules, OR'd together. `weekDays` holds 0-6 (0=Sunday). `alwaysOn`
rules are active for the whole `[startMinuteOfDay, endMinuteOfDay)` window
(wrapping past midnight if `endMinuteOfDay <= startMinuteOfDay`); `interval`
rules split that window into `intervalsPerWindow` equal slices and are active
for the first `durationMinutes` of each slice (a stateless duty-cycle read,
not an edge-triggered pulse). Deliberately has no `deps` — a schedule has no
dependencies of its own, it's other devices (`auto_switch`) that depend on
it. **`isActive()`/`timeValid()` are never exposed over `runtime`** — nothing
ever marks a `ScheduleDevice` runtime-dirty, so a REST/WS snapshot would go
silently stale while a tab stays open. The frontend computes an on/off
preview client-side instead, against the browser's own clock (see
`portal-spa/src/models/devices/schedule-preview.ts`).

### Auto Switch

```ts
type AutoSwitchMode = 'off' | 'on' | 'auto' | 'paused'

interface AutoSwitchConfig extends BaseDeviceConfig {
  pauseDurationSeconds: number
}

interface AutoSwitchRuntime extends BaseDeviceRuntime {
  output?: {
    mode?: AutoSwitchMode
    paused?: boolean
    pausedUntilMs?: number
    conditionsSatisfied?: boolean
    state?: boolean
  }
}
```

Wraps one required `switch` dependency (the real switch it drives) and a
bounded list (max 6) of optional `condition` dependency links, each carrying
`invert` (default `false`). A `condition` link may point at anything that
exposes a boolean status: a `schedule`, a `gpio_switch`, a
`port_expander_switch`, or another `auto_switch` (itself provides both
`switch` and `condition`, so auto switches can chain). In `auto` mode the
target follows the logical AND of every condition's `isActive() != invert`
reading — an empty condition list means `auto` never turns the target on
(the safe default, not vacuous truth). `conditionsSatisfied` in `runtime`
reflects that live AND result (renamed from the single-schedule-only
`scheduleActive` field this replaced). `mode` is a flat
off/on/auto/paused state (mirrors a prior personal project's
`ScheduledSwitchMode`) — `paused` is only reachable from `auto` and always
forces the target off for `pauseDurationSeconds`, then reverts to `auto`.
Mode changes go through the `setMode` command
(`{ command: 'setMode', mode: 'auto' | 'pause' }`), routed as
`DeviceCommandType::Custom`; plain on/off still uses the ordinary
`setOutput` command shared with every other `switch`-role device. See
`docs/schedule-and-auto-switch.md` for the full architecture.

### Binary Sensor

```ts
type GpioPullMode = 'none' | 'pullup' | 'pulldown'

interface BinarySensorConfig extends BaseDeviceConfig {
  gpioPin: number
  pullMode: GpioPullMode
  inverted: boolean
  debounceMs: number
}

interface BinarySensorRuntime extends BaseDeviceRuntime {
  output?: {
    active?: boolean
    rawLevel?: boolean
    hasReading?: boolean
  }
}
```

A debounced digital GPIO input (float switch, leak probe, door contact, ...).
Deliberately has no `deps` — it is a leaf `condition`-role provider that other
devices (`auto_switch`, `dosing_pump`) depend on. The raw pin level must stay
stable for `debounceMs` before `active` flips (`active = rawLevel !=
inverted`); `hasReading` is `false` until the first successful read after
boot/reconfigure. GPIO pins 34-39 are input-only and have no internal pull
resistors — setting `pullMode` to anything but `'none'` on one of them is
rejected with `INVALID_CONFIG`. Unlike `schedule`, this device marks itself
runtime-dirty on every debounced flip, so `output` is a live, WS-pushed value,
never a stale snapshot.

### Dosing Pump

```ts
type DosingScheduleMode = 'daily' | 'weekly'
type DosingRunState = 'idle' | 'dosing'
type DosingDoseType = 'schedule' | 'manual' | 'calibration'
type DosingContainerStatus = 'normal' | 'warning' | 'critical'

interface DosingPumpDoseConfig {
  time: string // "HH:mm"
  amountMl: number
}

interface DosingPumpConfig extends BaseDeviceConfig {
  dosingSpeedMlPerSec: number
  container: {
    capacityMl: number
    thresholdPercent: number
    blockAutoWhenEmpty: boolean
  }
  schedule: {
    mode: DosingScheduleMode
    everyDays: number // daily mode only, 1-30
    daysOfWeek: number[] // weekly mode only, 0-6 (0=Sunday)
    anchorDay: number // daily mode only, local days-since-1970 cycle anchor
    doses: DosingPumpDoseConfig[] // up to 16, sorted by unique time
  }
}

interface DosingPumpRuntime extends BaseDeviceRuntime {
  output?: {
    state: DosingRunState
    autoMode: boolean
    timeValid: boolean
    doseType?: DosingDoseType // only while state is "dosing"
    dosingTargetMl?: number
    dosedMl?: number
    dosingRemainingSec?: number
    dosingTotalSec?: number
    lastRunDosedMl: number
    todayDosedMl: number
    todayTargetMl: number
    nextDoseAt?: number // local-flavored epoch seconds
    nextDoseAmountMl?: number
    lastDose?: { at: number; type: 'schedule' | 'manual'; amountMl: number }
    daysLeft?: number
    container: {
      capacityMl: number
      currentMl: number
      percent: number
      empty: boolean
      sensorPresent: boolean
      status: DosingContainerStatus
    }
    skipNext: boolean[] // one entry per schedule dose, in order
  }
}
```

A peristaltic dosing pump channel. `config.deps` must contain one `switch`
dependency link (the pump's motor relay/MOSFET) and may contain one
`condition` dependency link (an optional low-level sensor, e.g.
`binary_sensor`, with `invert`). Run duration is computed from the calibrated
flow rate: `runSeconds = amountMl / dosingSpeedMlPerSec`. `daily` mode fires
every `everyDays` days, phase-anchored at `anchorDay` (send
`floor(Date.now() / 86400000)` whenever the client changes `everyDays`);
`weekly` mode fires on the days listed in `daysOfWeek`. The container's
current volume, `autoMode`, today's totals, and skip-next flags are volatile
runtime state, not part of `config` — they change through dedicated commands
below, never through `updateConfig`.

Dosing pump commands, in addition to `updateConfig`/`setDeps`/`delete`:

```ts
interface DosingPumpCommandRequest {
  command: 'setMode' | 'startDose' | 'stopDose' | 'setVolume' | 'skipNext'
  mode?: 'auto' | 'manual' // setMode
  amountMl?: number // startDose, 0.01-655.35
  logging?: boolean // startDose; default true. false = calibration run, excluded from totals and the dose journal
  volumeMl?: number // setVolume, >= 0 (a container refill or correction)
  doseIndex?: number // skipNext, must address an existing schedule dose
  skip?: boolean // skipNext, default true
}
```

- `setMode` toggles auto/manual, routed as `DeviceCommandType::Custom` (the
  same bridge `auto_switch`'s `setMode` uses).
- `startDose` is rejected while a run is already active. A scheduled dose
  marks its fired/skip-next bookkeeping the moment the run **starts**, not
  when it completes — a reboot mid-run never re-fires the same dose, but the
  un-run remainder of that dose is lost, not resumed.
- `stopDose` is idempotent; stopping an idle pump is a no-op. The amount
  actually dispensed (which may be less than requested if stopped early) is
  reported in `runtime.output.lastRunDosedMl`.
- Doses whose 5-minute grace window elapses without running (device busy,
  disabled, or powered off) are dropped for that day, never run late.

## Dose Journal

### `GET /api/dosejournal`

Query params: `deviceId` (optional; a specific pump, or every pump when
omitted/`0`) and `periodDays` (optional, default `7`, accepts `1`-`365`).
Returns dosing history newest-first, capped at 1000 entries. Only doses
logged with `logging: true` (scheduled doses and non-calibration manual
doses) appear — calibration runs (`startDose` with `logging: false`) are
never journaled. Entries persist in a LittleFS-backed ring buffer independent
of the device registry and survive reboots; if the system clock has never
synced, the endpoint returns everything the ring still holds rather than
filtering by `periodDays`.

```ts
interface DoseJournalEntry {
  at: number // local-flavored epoch seconds
  type: 'schedule' | 'manual'
  amountMl: number
}

interface DoseJournalResponse {
  success: true
  entries: DoseJournalEntry[]
}
```

## Device Registry

### `GET /api/devices`

Returns the complete device registry snapshot.

```ts
interface DeviceRegistryResponse {
  success: true
  registryRevision: number
  devices: DeviceRecord[]
}
```

### `GET /api/devices/:id`

Returns a single device snapshot.

```ts
interface DeviceDetailResponse {
  success: true
  registryRevision: number
  device: DeviceRecord
}
```

> Display devices (`ssd1306`, `st7735`): the `config` object does **not** include `layout`. The
> layout can be large (bitmap data) and is only needed by the designer, so it is served separately
> by `GET /api/devices/:id/layout` and is still saved through `updateConfig` with `config.layout`.
> See [Display Layout Persistence](./oled-display-layout.md) for the write/read flow and storage
> model. SSD1306 bus identity is carried only by `config.deps` using role `i2c_bus`; the removed
> `config.i2cBusDeviceId` field is rejected by the current API.

### `GET /api/devices/:id/layout`

Returns the display layout for a display device (404 for non-display devices). With `?page=<index>`
only that single page is returned in `pages`. Response is chunked.

```ts
interface DeviceLayoutResponse {
  success: true
  schemaVersion: number
  activePageId: string
  pages: Array<Record<string, unknown>> // one page object per page (id, name, order, widgets[])
}
```

### `POST /api/devices`

Creates a device. The request must contain the public `typeName` and a complete
persisted `config` object for that device type.

```ts
interface DeviceCreateRequest<TConfig extends BaseDeviceConfig = BaseDeviceConfig> {
  typeName: string
  config: TConfig
}
```

Example:

```json
{
  "typeName": "gpio_switch",
  "config": {
    "name": "GPIO Relay",
    "enabled": true,
    "deps": [],
    "restorePreviousState": false,
    "startupState": "off",
    "safeState": "disabled",
    "inverted": false,
    "gpioPin": 4
  }
}
```

Response:

```ts
interface DeviceMutationResponse {
  success: true
  registryRevision: number
  device?: DeviceRecord
}
```

### `POST /api/devices/:id/command`

Executes a structured command against a device.

```ts
interface DeviceCommandRequest {
  deviceId?: number
  command: 'delete' | 'updateConfig' | 'scan' | 'setOutput'
  state?: boolean | number
  config?: Record<string, unknown>
  deps?: DeviceDependencyLink[]
}
```

Command rules:

- `delete` and `scan` require no extra field.
- `setOutput` requires a boolean `state` for switch outputs or a percentage number
  from `0` through `100` for analog outputs.
- `updateConfig` requires `config`. `config` may be a partial object containing
  only the keys that changed — the device applies a partial merge against its
  current persisted config, so any field omitted from `config` keeps its
  current value (omitted fields are never reset to compiled defaults). This
  includes `name` and `enabled`: there is no separate rename/enable/disable
  command — renaming a device or toggling it on/off is just another
  `updateConfig` field change. Renaming rejects with `INVALID_CONFIG` if
  another device already has that name; flipping `enabled` drives the
  device's lifecycle (start/stop) the same way a dedicated command used to.
  `deps`, when included, is not merged: it must be the full current
  dependency snapshot for the device (all roles), matching existing
  behavior.
- `deviceId`, when provided, must match the `:id` path parameter.
- Public clients must not send packed `payload` strings or binary config blobs.

Examples:

```json
{ "command": "setOutput", "state": "on" }
```

```json
{
  "command": "updateConfig",
  "config": {
    "name": "Water Temperature",
    "enabled": true,
    "deps": [{ "role": "onewire_bus", "deviceId": 670845751 }],
    "address": "28FF641D621603AD",
    "resolution": 12,
    "unit": "celsius",
    "pollMs": 5000,
    "reportDeltaCelsius": 0.25,
    "reportAlways": false
  },
  "deps": [{ "role": "onewire_bus", "deviceId": 670845751 }]
}
```

Partial update — only `reportDeltaCelsius` changed; all other fields
(`address`, `resolution`, `unit`, `pollMs`, `reportAlways`, `name`, `enabled`)
are left untouched server-side:

```json
{
  "command": "updateConfig",
  "config": { "reportDeltaCelsius": 0.5 }
}
```

Rename only (rejects with `INVALID_CONFIG` if another device already has this name):

```json
{
  "command": "updateConfig",
  "config": { "name": "Water Temperature (kitchen)" }
}
```

Disable/enable only (drives the device's start/stop lifecycle the same way a
dedicated command used to):

```json
{
  "command": "updateConfig",
  "config": { "enabled": false }
}
```

### `DELETE /api/devices/:id`

Deletes a device. On dependency conflicts, the error body includes
`dependentDeviceIds`.

### `POST /api/devices/flush`

Forces pending registry persistence to be flushed.

```ts
interface DeviceFlushResponse {
  success: true
  registryRevision: number
}
```

## WiFi

### `GET /api/wifi/status`

```ts
interface WifiStatusResponse {
  success: true
  wifiStatus: 'connected' | 'connecting' | 'disconnected' | 'failed' | 'idle' | 'ble_config'
  stationIp: string
  setupApIp: string
}
```

### `GET /api/wifi/scan`

Returns `202` while scanning and `200` when results are ready.

```ts
interface WifiScanNetwork {
  ssid: string
  rssi: number
  channel: number
}

interface WifiScanResponse {
  success?: true
  status: 'ok' | 'scanning'
  networks?: WifiScanNetwork[]
}
```

### `POST /api/wifi/configure`

```json
{ "ssid": "Network", "password": "optional" }
```

Accepted response:

```json
{ "success": true, "status": "accepted" }
```

### `DELETE /api/wifi/configure`

Clears stored WiFi credentials.

```json
{ "success": true, "status": "accepted", "action": "clear_wifi_credentials" }
```

### `POST /api/wifi/ble-config`

Starts BLE WiFi provisioning when available.

```json
{ "success": true, "status": "accepted", "action": "start_ble_config" }
```

## Dashboard Layout

### `GET /api/dashboard/layout`

```ts
type DashboardLayoutWidgetRecord = [deviceId: number, x: number, y: number, w: number, h: number]

interface DashboardLayoutPanelRecord {
  id: string
  name: string
  order: number
  widgets: DashboardLayoutWidgetRecord[]
}

interface DashboardLayoutRecord {
  schemaVersion: number
  activePanelId: string
  panels: DashboardLayoutPanelRecord[]
}

interface DashboardLayoutResponse {
  success: true
  revision: number
  layoutDefaulted?: boolean
  layout: DashboardLayoutRecord
}
```

### `PUT /api/dashboard/layout`

Accepts either a raw `DashboardLayoutRecord` or `{ "layout": DashboardLayoutRecord }`.
Returns `204 No Content` on success.

## OTA

The current `esp32dev` build compiles OTA out for flash budget (no
`WITH_WEB_OTA`), so `enabled` is always `false`. The SPA hides the OTA nav
entry, the Overview tile/card, and only shows the dedicated OTA page's
disabled-state alert when `enabled` is `false` — it does not treat this as an
error.

### `GET /api/ota/status`

```ts
interface OtaStatusResponse {
  success: true
  enabled: boolean
  freeSketchSpace: number
  hasError: boolean
}
```

### `POST /api/ota`

Uploads firmware through the request body. On success, the registry is flushed,
the controller returns a closing response, and the device reboots.

```json
{ "success": true, "status": "ok", "rebooting": true }
```

## MQTT

Optional feature, compiled in only with `-DWITH_HOME_ASSISTANT` (see
`docs/mqtt-home-assistant.md`). `enabled` in `MqttStatusResponse` reflects
whether the flag was compiled in, not the runtime on/off toggle (that's
`MqttSettingsRecord.enabled`) — the frontend uses the former to gray out the
settings page and the latter to drive the enable switch.

### `GET /api/mqtt/status`

```ts
interface MqttStatusResponse {
  success: true
  enabled: boolean // compiled in (WITH_HOME_ASSISTANT)
  connected: boolean
  waitingForStation: boolean
  host: string
  port: number
  useTls: boolean
  clientId: string
  hasCaCert: boolean
}
```

### `GET /api/mqtt/settings` / `PUT /api/mqtt/settings`

`PUT` accepts a partial object — only provided fields are updated. `haNodeId`
defaults (once, on first boot) to `<deviceName>-<macSuffix>` sanitized to
`[a-zA-Z0-9_-]`; changing it after devices have been announced to Home
Assistant creates a **new** device there (old entities/history are orphaned).

```ts
interface MqttSettingsRecord {
  enabled: boolean
  host: string
  port: number
  useTls: boolean
  clientId: string
  username: string
  password: string // write-only; GET responses omit it (see passwordRedacted)
  passwordRedacted: boolean // GET only
  haDiscoveryPrefix: string
  haNodeId: string
  haNodeName: string
  hasCaCert: boolean
}
```

### `POST /api/mqtt/ca-cert` / `DELETE /api/mqtt/ca-cert`

Multipart upload (field name `cert`) of a PEM-encoded CA certificate, used
only when `useTls` is `true`. `DELETE` clears the stored certificate. Both
respond with the same shape as `GET /api/mqtt/status`.

### Per-device Home Assistant opt-in

Devices are **not** published to Home Assistant by default. Each device's
JSON envelope gains a generic `ha` block (present only when
`WITH_HOME_ASSISTANT` is compiled in), alongside `record`/`config`/`runtime`:

```ts
interface DeviceHaSettings {
  enabled: boolean
  name: string // override; empty means "use config.name"
  effectiveName: string // name override if set, else config.name
}
```

Toggling it is a `cmd` on the device (`POST /api/devices/:id`, mirroring
`setOutput`/`setDeps`):

```json
{ "command": "setHaSettings", "haEnabled": true, "haName": "Pump" }
```

## System

### `POST /api/system/restart`

Flushes the registry and schedules a controller restart.

```ts
interface SystemRestartResponse {
  success: true
  rebooting: boolean
}
```

### `GET /api/system/version`

```ts
interface SystemVersionResponse {
  success: true
  version: string
  buildDate: string
}
```

### `GET /api/system/status`

Flash partition table, LittleFS usage, NVS usage, heap, and chip info — used
by the Overview page's storage/hardware cards. `filesystems` enumerates the
two LittleFS-formatted partitions (`littlefs` for UI assets, `devdata` for the
dosing pump journal, see `docs/dosing-pump.md`); `mounted: false` means that
partition failed to mount (fine for `devdata` — dosing devices tolerate a
missing journal by dropping records) rather than an endpoint error.
`partitions` enumerates the full flash partition table from `my_partitions.csv`
(`nvs` in that list is the raw partition size, not entry usage — see `nvs`
below for that). `resetReason` matches one of the keys backing the SPA's
`system.resetReasons` i18n block.

```ts
interface PartitionInfo {
  label: string
  type: string // 'app' | 'data' | 'other'
  subtype: string // e.g. 'factory', 'spiffs', 'nvs', 'ota'
  offset: number
  sizeBytes: number
}

interface FilesystemUsage {
  label: string
  mounted: boolean
  totalBytes: number
  usedBytes: number
}

interface SystemStatusResponse {
  success: true
  chip: {
    model: string
    revision: number
    cores: number
    cpuFreqMhz: number
    flashSizeBytes: number
  }
  uptimeSeconds: number
  resetReason: string
  heap: {
    totalBytes: number
    freeBytes: number
    minFreeBytes: number
    maxAllocBytes: number
  }
  sketch: {
    usedBytes: number
    partitionBytes: number
  }
  partitions: PartitionInfo[]
  filesystems: FilesystemUsage[]
  nvs: {
    usedEntries: number
    freeEntries: number
    totalEntries: number
    namespaceCount: number
  }
}
```

### `GET /api/system/time`

Live sync status. `localTimeIso8601`/`utcOffsetMinutes`/`timezoneAbbrev` are
only present once `synced` is `true` (no reliable wall-clock time exists
before the first successful sync). `source` reflects how the current time
was obtained: `"ntp"` after a successful NTP exchange, `"manual"` after a
`POST /api/system/time` override, or `"rtc"` when the time was seeded/
corrected from the active `rtc_ds3231` device (`RtcSyncCoordinator`) — at
boot before NTP has ever synced, or as a fallback if NTP sync has been
failing for a long time. Unlike `"ntp"`/`"manual"`, an `"rtc"` source does
not count as an authoritative sync internally, so NTP keeps retrying
independently and the status can flip back to `"ntp"` once it succeeds.
`enabled` is the persisted NTP on/off toggle (see settings below); when
`false`, `waitingForStation` stays `false` too since the sync state machine
isn't attempting to run at all.

```ts
interface TimeStatusResponse {
  success: true
  enabled: boolean
  synced: boolean
  waitingForStation: boolean
  ntpServer: string
  timezoneId: string
  syncIntervalSeconds: number
  source: 'ntp' | 'manual' | 'rtc'
  currentEpochUtc?: number
  lastSyncEpochUtc?: number
  localTimeIso8601?: string
  utcOffsetMinutes?: number
  timezoneAbbrev?: string
}
```

### `POST /api/system/time`

Sets the system clock immediately from a caller-supplied timestamp,
regardless of whether NTP sync is enabled — used for manual time entry (e.g.
no internet access yet) or a one-off correction. Accepts full ISO 8601,
either with an explicit UTC offset or a `Z` suffix for UTC; a bare
`YYYY-MM-DDTHH:MM` (no offset) is treated as UTC. Responds with the same
shape as `GET /api/system/time` (`source` will be `"manual"`).

```ts
interface SetTimeRequest {
  iso8601: string // e.g. "2024-03-05T12:45:00+02:00" or "2024-03-05T12:45:00Z"
}
```

### `GET /api/system/time/settings` / `PUT /api/system/time/settings`

`PUT` accepts a partial object — only provided fields are updated. Applying
new settings forces an immediate resync instead of waiting out the normal
periodic interval (this also makes an unchanged `PUT {}` body a de facto
"sync now" action). Setting `enabled: false` stops the sync state machine
from ever leaving its wait state; it does not clear the last-known time.

```ts
interface TimeSettingsRecord {
  enabled: boolean
  ntpServer: string
  timezoneId: string // IANA-style id from the SPA-bundled timezone catalog
  syncIntervalSeconds: number
}
```

### Timezone catalog (no endpoint)

There is no REST endpoint for the timezone list. The display names are static
and are bundled in the SPA (`portal-spa/src/data/timezones.ts`, mirroring the
repo-root `timezones.json`); `fetchTimezones()` resolves them locally without a
network round-trip. The firmware keeps only the ids + DST rules
(`src/time/TimeZoneTable.cpp`) to validate a submitted `timezoneId`. Keep the id
set/order in sync across `timezones.json`, the SPA catalog, and the C++ table.

```ts
interface TimezoneCatalogEntry {
  id: string
  name: string
}
interface TimezoneCatalogResponse {
  success: true
  timezones: TimezoneCatalogEntry[]
}
```

## Device Setup Transfer

See `docs/backup-and-restore.md` for the full bundle format, hand-editing
rules, and external auto-backup recipes.

### `GET /api/device-setup/export`

Returns `application/x-ndjson` (transfer schema version 3). The first line is a
`transfer_envelope`; each device line contains setup data only: identity plus
`configVersion` inside `record`, and the persisted `config` including deps. Display
adapters append ordered `layout_begin`, `layout_page`, `layout_widget`, and `layout_end`
records immediately after their device. Runtime state is omitted. When a
non-default dashboard layout exists, a trailing `dashboard_layout` line carries
it as JSON.

### `POST /api/device-setup/import`

Accepts a multipart upload field named `bundle` containing an exported (or
hand-edited) NDJSON bundle. Schema versions 1, 2, and 3 are accepted. Device
configs are parsed by the same per-type JSON parsers as REST create and
re-encoded at the current config version; a config that no longer parses
reports a per-device error that names the device, its type, and the version
gap. Import atomically replaces the device registry; display and dashboard
layouts are applied afterwards, with non-fatal problems reported in
`warnings`.

```ts
interface DeviceSetupTransferResponse {
  success: true
  registryRevision: number
  deviceCount: number
  warnings?: string[]
}
```

## Error Codes

Known error codes include `BAD_ARGS`, `BAD_PARAMS`, `BAD_JSON`, `BOUNDS_EXCEEDED`,
`BUSY`, `DEPENDENT_DELETE`, `DUPLICATE_DEVICE_ID`, `INTERNAL`,
`INVALID_COMMAND`, `INVALID_CONFIG`, `INVALID_DEVICE_ID`,
`INVALID_RELATIONSHIP`, `INVALID_VERSION`, `NOT_FOUND`, `OTA_FAILED`,
`STORAGE_ERROR`, and `UNSUPPORTED_TYPE`.

MQTT-specific codes (`## MQTT`): `HOST_REQUIRED`, `HOST_TOO_LONG`,
`PORT_INVALID`, `CLIENT_ID_REQUIRED`, `CLIENT_ID_TOO_LONG`,
`USERNAME_TOO_LONG`, `PASSWORD_TOO_LONG`, `DISCOVERY_PREFIX_INVALID`,
`NODE_ID_REQUIRED`, `NODE_ID_TOO_LONG`, `NODE_ID_INVALID_CHARACTERS`,
`NODE_NAME_TOO_LONG`, and `CERT_TOO_LARGE`.

Time/NTP-specific codes (`## System`): `NTP_SERVER_TOO_LONG`,
`TIMEZONE_INVALID`, `SYNC_INTERVAL_INVALID`, and `INVALID_TIME` (malformed
`iso8601` on `POST /api/system/time`).

Error example:

```json
{
  "success": false,
  "code": "BAD_ARGS",
  "error": "name is required"
}
```

## Compatibility Notes

Legacy snake_case command names such as `update_config`, `set_status`, and
`set_output` are not part of the public REST contract. Backend code may accept
them temporarily during migration, but frontend code and tests should use the
camelCase command names documented here.

Legacy public create payloads using `type`, `typeId`, or `type_id` are not part
of this contract. Clients must use `typeName`.
