# Analog Output

Four device types model one physical PWM output and let it be composed with fade
smoothing, a daily schedule, and multi-channel grouping without ever leaving the
registry's existing dependency-graph model: `analog_output` (`LedcAnalogOutputDevice`,
the LEDC hardware backend), `fade_analog_output` (`FadeAnalogOutputDevice`),
`scheduled_analog_output` (`ScheduledAnalogOutputDevice`), and `analog_output_composer`
(`AnalogOutputComposerDevice`). See [REST API Contract](rest-api-contract.md) for the
public config/runtime JSON shape of each type.

## Shared output runtime (`src/devices/output/`)

Switch and analog output are two instances of the same typed contract, not two
unrelated device families:

- `IOutputRuntime<ValueType>` (`src/devices/core/DeviceTypes.h`) defines
  `currentOutputState()` / `requestOutputState(ValueType, now)`. `ISwitchOutputRuntime`
  specializes it with `ValueType = bool` (role `Switch`); `IAnalogOutputRuntime`
  specializes it with `ValueType = uint16_t` (role `AnalogOutput`).
- `AbstractOutputDevice<ValueType, RuntimeInterface>` (`src/devices/output/AbstractOutputDevice.h`)
  owns the entire lifecycle both families share: startup-state selection, optional
  retained-state restore (`OutputDeviceRetainedStateRecord<ValueType>`), command-driven
  state requests, reconfiguration, disable → safe-state → hardware release, and fault
  handling. `GpioSwitchDevice` derives from `AbstractOutputDevice<bool, ISwitchOutputRuntime>`;
  `AnalogOutputDeviceBase` derives from `AbstractOutputDevice<uint16_t, IAnalogOutputRuntime>`.
  Concrete backends only implement the hardware-facing hooks (`configureHardware`,
  `applyHardwareOutput`, `releaseHardware`, `invertedState`, `stateIsValid`,
  `parseOutputCommand`).
- `OutputDeviceConfigV1<ValueType>` (`src/devices/output/OutputDeviceConfig.h`) owns the
  persisted fields common to every output config: `restorePreviousState`, `startupState`,
  `safeState`, `inverted`. Concrete configs (e.g. `LedcAnalogOutputDeviceConfigV1`) extend
  it with only their hardware-specific fields (pin, channel, frequency, ...).

## One output = one scalar 0..4095

Every analog output device — the LEDC backend and every decorator on top of it —
represents exactly one physical or logical channel as a `uint16_t` in `0..4095`
(`kAnalogOutputLevelMax`). Firmware never stores or computes in float or in percent;
`OutputDeviceValueCodec<uint16_t>` is the single place that converts between the
internal `0..4095` range and the `0..100` percent values used at the REST/HA/SPA
boundary (config fields like `startupState`/`safeState`/`maxStep`/schedule point
`state`, and runtime `output.state`, all go through this codec). Multiple physical
outputs are always multiple registry devices, never one multi-channel device — a
discarded early LEDC prototype supported multiple channels per device but was never
persisted by a released runtime, so there was no compatibility contract to preserve
when it was replaced by the current one-output-per-device model.

## Decorator chain (`src/devices/analog/`)

`FadeAnalogOutputDevice` and `ScheduledAnalogOutputDevice` each consume exactly one
`analog_output`-role dependency (`AnalogOutputDependencyConfig.targetDeviceId` on the
SPA side) and provide that same `AnalogOutput` role themselves
(`AnalogOutputDecoratorDeviceBase` implements `IAnalogOutputRuntime` by forwarding to
whatever it targets). This lets them stack: LEDC → Fade → Scheduled is a normal,
supported chain, built purely from existing registry dependency links with no new
graph or event-bus mechanism. Role validation accepts any compatible ordering; the
registry additionally treats a controlling analog dependency as **exclusive** — it
rejects wiring a second controller onto the same target device, so a chain can't
silently fight itself over one physical output.

### Fade (`fade_analog_output`, type 21, `FadeAnalogOutputDeviceConfigV1`)

- Config: `maxStep` (percent per interval, codec-converted; default `41`⁄4095 ≈ 1%) and
  `stepIntervalMs` (`1..60000`, default `200`).
- The ramp is a bounded integer step-and-interval controller, independent of the LEDC
  backend. A delayed tick computes elapsed intervals arithmetically and clamps the
  result instead of running an unbounded catch-up loop, so a long gap between ticks
  can't cause a burst of queued steps.
- Runtime (`FadeAnalogOutputOutputSnapshot`) additionally reports `targetState` and
  `transitioning` so the SPA can show the settling animation.

### Scheduled (`scheduled_analog_output`, type 22, `ScheduledAnalogOutputDeviceConfigV2`)

- Config: `points`, up to `kMaxScheduledAnalogOutputPoints = 10` entries of
  `{ deleted, minuteOfDay (0..1439), state (percent) }`, stored as a fixed plain-C
  array with an explicit `deleted` tombstone per slot rather than a variable-length
  list. V2 requires at least one active point and rejects duplicate active
  `minuteOfDay` values; a legacy V1 blob (no minimum-point requirement) decodes and
  migrates to V2 on load (`ScheduledAnalogOutputDeviceConfigV2::migrateFrom`).
- Runtime interpolation between points uses signed 64-bit integer arithmetic and
  ignores deleted slots.
- Mode is `AnalogOutputMode` (`Off` / `Manual` / `Scheduled`, `src/devices/core/DeviceTypes.h`).
  Only the mode and the manual level are persisted/retained — the calculated
  scheduled value is never persisted. `Off`, an invalid wall-clock time, and an empty
  effective schedule all resolve the requested output to zero rather than holding
  the last value.

### Composer (`analog_output_composer`, type 23, `AnalogOutputComposerDeviceConfigV1`)

- Provides its own role, `AnalogOutputGroup` (`IAnalogOutputGroupRuntime`), instead of
  `AnalogOutput` — it does not pretend to be one scalar output. It consumes an ordered,
  repeated list of `analog_output`-role dependencies (`targetDeviceIds: number[]` on the
  SPA side, encoded as one `analog_output` dependency link per channel).
- Composer owns and retains one Off/Manual/Scheduled mode for the whole group. It
  applies that mode to every compatible (`scheduled_analog_output`) child dependency
  at startup, on an explicit mode command, and whenever a child's own mode diverges
  from the group's. An `Off` group command zeros every channel; switching between
  Manual and Scheduled leaves ordinary (non-scheduled) analog output children
  unchanged.
- Config-write failures across children are partial-tolerant: successful channel
  writes are kept, failed channels are identified, and their authoritative config is
  reloaded rather than rolling back the whole batch.

## SPA schedule graph (`portal-spa`)

One shared chart component renders both a single scheduled channel and a composed
multi-channel schedule, reused across the dashboard card (compact, read-only) and the
device edit dialog (full, editable). In edit mode it exposes draggable point handles,
duplicate-time prevention, and an optional snapping grid (15 min / 5% by default,
disable for 1 min / 1% precision); right-clicking a point offers Delete/Insert
before/Insert after, right-clicking empty space adds a point at the cursor. A
SPA-only daylight layer computes local sunrise/sunset from the current date, browser
timezone offset, and (if the user has granted it) browser geolocation cached in
local storage, falling back to a flat 06:00–18:00 band otherwise; the compact chart
shows just the gradient, the full chart adds sunrise/sunset legend chips and a
location button. The composer's own SPA editor edits child schedules directly on this
shared chart rather than keeping a second manual-entry table, and exposes manual
per-channel control as one row of vertical sliders (requested level, with actual
level as a secondary readout).

## Home Assistant integration

See [MQTT + Home Assistant](mqtt-home-assistant.md) ("Current scope") for the
discovery entities each of these device types publishes.
