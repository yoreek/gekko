# Schedule + Auto Switch

Two device types that let a switch turn on/off based on a bounded list of AND
conditions: `schedule` (`ScheduleDevice`) holds a time-of-day/weekday rule set
and reports whether it's currently active; `auto_switch` (`AutoSwitchDevice`)
wraps a real switch and follows the logical AND of every attached `condition`
dependency (each optionally inverted), with a manual Off/On override and a
temporary pause.

Loosely modeled on a prior personal project (ReefDuino,
`.pio/libdeps/esp32dev_reefduino/ReefDuino/src/schedule`), but simplified: no
edge-triggered "pulse" timing. The per-switch AND-condition dependency list
(ReefDuino's `std::vector<ScheduledSwitchDepsRule>`) is implemented, generalized
beyond schedules to any device exposing `IStatusRuntime`.

## `ScheduleDevice` (`src/devices/schedule/`)

- Config: up to `kMaxScheduleRules = 4` rules (`ScheduleRuleV1`), each with a weekday
  bitmask (`weekDays`, bit0=Sunday..bit6=Saturday), a time-of-day window
  (`startMinuteOfDay`/`endMinuteOfDay`, 0-1439), and a mode:
  - `AlwaysOn` — active for the whole window.
  - `Interval` — splits the window into `intervalsPerWindow` equal slices and treats the
    first `durationMinutes` of each slice as "on" (a stateless duty-cycle read, not a
    triggered pulse — see "What we deliberately dropped" below).
- **Minute precision only** — there is no seconds-of-day concept anywhere in a rule.
  `ScheduleDeviceConfig.cpp`'s JSON parsing rounds any non-integer-minute input to the
  nearest whole minute (`parseMinuteOfDay`/`parseDurationMinutes`), so this is enforced
  at the one place values get saved, not left to every caller to get right.
- `isActiveAt(const DateTime& currentTime) const` is the portable, pure rule-evaluation
  core (`isSuitableDayOfWeek`/`isSuitableTimeRange`/`isSuitableInterval`/`isRuleActive`
  in `ScheduleDevice.cpp`) — OR across all enabled rules. No wall-clock dependency, so
  this is what native unit tests exercise directly (`test_schedule_device.cpp`).
- `isActive()`/`timeValid()` (the `IScheduleRuntime` interface `AutoSwitchDevice` reads)
  call `DateTime::current()` directly — Arduino-only, guarded by
  `#if defined(ARDUINO) && !defined(UNIT_TEST)`. There is no coordinator/registry scan
  pushing time into `ScheduleDevice`; it pulls the current time itself, on demand, the
  moment something asks.
- A schedule younger than 2020 (`kScheduleMinValidYear`) is treated as "no clock yet" —
  mirrors the reference's `AbstractScheduledSwitch::isValidEpoch()` year check, since a
  dead-battery RTC or a never-synced clock commonly reads back as some
  power-on-reset/epoch-zero date.
- `isActive()` caches its result keyed by `DateTime::current().unixtime() / 60` (a
  monotonically increasing minute counter — deliberately *not* `weekday()` +
  `minutesOfDay()`, since `minutesOfDay()` alone repeats every day and would collide
  across a midnight boundary). The result can only change on a minute boundary (every
  field a rule reads is minute-granular), so this avoids re-walking the rule list on
  every call. This matters because several `AutoSwitchDevice`s commonly share one
  `ScheduleDevice`, each polling it on its own 1s tick — multiple calls per second
  against the same minute is the normal case, not an edge case. The cache is dropped on
  `applyConfig()` (rules changed) and whenever the clock looks implausible.
- **`isActive()`/`timeValid()` are *not* exposed over REST/WS.** `ScheduleDeviceApiAdapter`
  deliberately omits an `output.active`/`output.timeValid` field. The reason: nothing
  ever calls `markRuntimeStateDirty()` on `ScheduleDevice` (its state machine has no
  reason to — it holds no other runtime state), so `DeviceRegistry::emitRuntimeStatusChanges()`
  never pushes a `StateChanged` WS event for it. A value read only via REST/initial WS
  snapshot would be a silent snapshot from whenever the page happened to load, which is
  actively misleading for something inherently time-dependent — a stale "Active" chip
  that never updates while the tab stays open is worse than no chip at all. The
  frontend instead computes an on/off preview client-side, straight from the rule
  config, against the *browser's* own clock:
  `portal-spa/src/models/devices/schedule-preview.ts`'s `isScheduleActiveAt()`/
  `describeScheduleStatus()` are a direct port of `isSuitableDayOfWeek`/
  `isSuitableTimeRange`/`isSuitableInterval`/`isRuleActive` above, plus a forward scan
  for the next on/off transition. This is an *estimate* — the browser's clock/timezone
  can differ from the device's configured one — so the UI labels it accordingly
  (`device.dialog.schedule.estimateNote`). `IScheduleRuntime::isActive()`/`timeValid()`
  themselves are untouched and still drive `AutoSwitchDevice`'s real behavior, which
  re-evaluates them directly in C++ every tick — that path was never affected by the
  WS-staleness problem to begin with.

## `AutoSwitchDevice` (`src/devices/switch/auto/`)

- Depends on exactly one required `DeviceRole::Switch` (the real switch it drives) and a
  bounded list (`kMaxAutoSwitchConditions = 6`) of optional `DeviceRole::Condition`
  dependencies, each carrying a per-link `invert` flag (`DeviceDependencyLink::invert`,
  stored in a wire byte that was previously reserved-and-discarded — no registry version
  bump needed). A condition link may point at anything implementing `IStatusRuntime`
  (`DeviceTypes.h`) — a `ScheduleDevice`, a `SwitchDeviceBase`-derived switch
  (`GpioSwitchDevice`/`PortExpanderSwitchDevice`), or another `AutoSwitchDevice` — each of
  those types declares `Condition` alongside its primary role in
  `DeviceTypeDescriptor::providedRoles` (a small bounded set, not a single scalar role;
  see the "role lives on the provider" comment in `DeviceTypes.h`). Itself **provides**
  both `DeviceRole::Switch` (`ISwitchOutputRuntime`) and `DeviceRole::Condition`
  (`IStatusRuntime`), so it shows up as an ordinary switch tile on the dashboard, can be
  used as a dependency by anything that accepts a switch (mirrors
  `PortExpanderSwitchDevice`'s dependency/provider duality), and can be chained as another
  `AutoSwitchDevice`'s condition.
- `refreshCapabilityCache()` resolves the target switch and every condition link once
  (on dependency wiring, not per tick), caching each condition's `IStatusRuntime*` +
  `invert` flag in a bounded array. `conditionsSatisfied()` then just loops that cached
  array each tick — `source->isActive() != invert` per entry, ANDed together. An **empty**
  condition list evaluates to `false` (not vacuously true), preserving the original
  single-schedule behavior of never turning on in `Auto` until something is actually
  attached. No new caching layer was needed for this: `ScheduleDevice::isActive()`
  already caches per-minute (see below) precisely because several `AutoSwitchDevice`s
  commonly share one schedule; a plain switch's `isActive()` is just an O(1) field read.
- Mode is one flat `AutoSwitchMode` — `Off` / `On` / `Auto` / `Paused` — mirroring
  ReefDuino's `ScheduledSwitchMode` (`TurnedOnMode`/`TurnedOffMode`/`ScheduledMode`/
  `PausedMode`) exactly: `Paused` is **not** a separate overlay flag sitting on top of
  `Auto`, it is just another value of the same field. There is no distinct "resume"
  concept or command — the same `"auto"` command that enters `Auto` from `Off`/`On` is
  also the only way out of `Paused` (mirrors the reference's `schedule()`, the single
  entry point into `ScheduledMode` from anywhere, including `PausedMode`).
  Mode — including `Paused` — is persisted as **retained state**
  (`AutoSwitchRetainedStateV1`, via `DeviceRetainedDataStore` — the same mechanism
  `SwitchDeviceBase` uses for its own output state), not inside the versioned config blob
  — avoids an NVS config-blob rewrite on every manual toggle. A reboot mid-pause resumes
  paused, with the correct remaining time, not silently back in `Auto`.
- **Surviving a reboot while paused** is the one place `AutoSwitchDevice` needs a wall
  clock: the live countdown (`pausedUntilMs_`) is `uptime()`-relative and always resets to
  0 on boot, so it can't be persisted directly. `AutoSwitchRetainedStateV1` additionally
  carries `pausedUntilEpoch` — a `DateTime::unixtime()` anchor, computed only at the
  save/load boundary (both `#if defined(ARDUINO) && !defined(UNIT_TEST)`, guarded by the
  same `kAutoSwitchMinValidYear = 2020` untrustworthy-clock check `ScheduleDevice` uses):
  `saveRetainedState()` converts the remaining `pausedUntilMs_` duration into a wall-clock
  timestamp before writing it out; `loadRetainedState()` does the reverse, converting the
  persisted timestamp back into a fresh `uptime()`-relative deadline for the new boot. If
  the wall clock isn't trustworthy yet when `loadRetainedState()` runs (no RTC, NTP not
  synced yet) or the persisted deadline already passed during the outage,
  it falls back to plain `Auto` rather than getting stuck paused forever. Everywhere else
  (the normal, non-reboot tick-driven elapse check in `applyDesiredOutput()`) stays plain
  `uptime()` arithmetic — no wall clock involved, and natively testable exactly like
  before.
- In `Auto` mode, the desired output follows `conditionsSatisfied()` (`Off` if no
  conditions are attached — a safe default). In `Off`/`On`, conditions are ignored.
- **`Paused`** (`AutoSwitchDevice::handleCommand()`'s `"pause"` command) is only accepted
  while `mode_ == Auto` — mirrors the reference's `pause()`, which only takes effect when
  `_state->mode == ScheduledMode` (a plain `Off`/`On` mode can't be paused; calling
  `handleCommand` with `"pause"` outside `Auto` returns `false` and is a no-op). While
  paused, the desired output is always `Off` (mirrors `PausedMode`'s `_toggle(false)`; it
  is not a "hold whatever state was current" freeze). Not configurable beyond
  `pauseDurationSeconds` (how long it lasts) — there's no setting for what state pause
  drives. Once the deadline elapses (checked in `applyDesiredOutput()`, mirrors the
  reference's `CHECK_PAUSE`), the device reverts to `Auto` — the only mode `Paused` is
  ever reached from, so there's nothing else to "remember" and restore.
- **Entering `Auto` or `Paused`** (from any other mode) always forces the target off on
  the very next tick before the schedule is consulted — mirrors the ReefDuino reference's
  `SWITCH_MODE`, which unconditionally `_toggle(false)`s whenever the state machine
  transitions into `ScheduledMode` or `PausedMode`, regardless of which mode it came from.
  This matters because the naive alternative — immediately reading `conditionsSatisfied()`
  on the same tick — would silently keep the target on if it happened to already match
  whatever a prior manual `On` override left it at, rather than genuinely handing control
  back to the conditions. `forceOffPending_` (`AutoSwitchDevice.cpp`, set inside
  `setMode()`) implements this: consumed by the *next* `applyDesiredOutput()` tick
  (forcing off and returning early), so the conditions are only actually read on the tick
  *after* that.
- Manual override: calling `requestOutputState()` with a boolean on/off state (either as a dependent's normal
  `ISwitchOutputRuntime` call, or via the portal's `setOutput` command from a dashboard
  toggle) sets the mode to `On`/`Off` directly — this exits `Paused` (or `Auto`) the same
  way any other `setMode()` call does. `Auto`/`Paused` are reached via a dedicated portal
  command, `setMode` (`{ command: 'setMode', mode: 'auto' | 'pause' }`), routed to
  `DeviceCommandType::Custom` — see `DeviceRegistryController::command()`. This is a
  generic free-text bridge: the controller just forwards `mode` verbatim to
  `handleCommand()`; it doesn't know or care what `AutoSwitchDevice` does with the string.

## Time: `DateTime::current()` / `DateTime::applyTimezone()`

This has **nothing to do with `NtpManager`**. `NtpManager` only ever writes the system
clock (`setTime()` from a successful NTP sync, a manual REST time-set, or an RTC seed);
it has no notion of timezone and doesn't need to exist at all for schedules/current time
to work.

- `src/time/DateTime.h`/`DateTimeTimezone.cpp` — `DateTime::current()`/`applyTimezone()`
  are the only two `DateTime` members gated `#if defined(ARDUINO) && !defined(UNIT_TEST)`;
  every other member (`year()`/`weekday()`/`minutesOfDay()`/...) is plain calendar
  arithmetic with zero Arduino dependency, so it stays native-testable.
  `DateTimeTimezone.cpp` is the **only** file in `src/time/` allowed to depend on
  `<Timezone.h>` (jchristensen/Timezone).
- A single persistent `Timezone` object (`g_systemTimezone`, file-scope static in
  `DateTimeTimezone.cpp`) is rewritten in place by `applyTimezone()` only when the
  configured zone actually changes — mirroring the ReefDuino reference's global
  `systemTimezone`. This matters because `Timezone::toLocal()` only recalculates its DST
  transition points when the calendar year changes; constructing a fresh `Timezone` on
  every call (the original approach here, before this was fixed) throws that cache away
  and forces the full recalculation on every single call, however often it's polled.
- `ConfigStore::save()` (`src/config/ConfigStore.cpp`) is the single hook that calls
  `DateTime::applyTimezone(config_.time.timezoneId.c_str())` — every config write (boot
  load, WiFi credential save, time settings save, ...) funnels through this one method,
  so it's the one place that needs to know about timezone sync at all. Nothing in
  `NtpManager` references timezones or `DateTime::applyTimezone()`.
- `g_systemTimezone` is guarded by a `portMUX_TYPE` critical section (same primitive
  `WifiManager` uses for its own cross-task state) because it's written from
  `ConfigStore::save()` and read from `DateTime::current()` on both the main loop
  (`ScheduleDevice::isActive()`, via `App::tick()`) and the async_tcp task
  (`TimeController`'s HTTP handler).

## What we deliberately dropped (vs. the ReefDuino reference)

- **Edge-triggered pulses / `lastStartedAt` / min-interval anti-chatter guard** — the
  reference's `_isSuitableMinInterval`/`_isDurationReached`/`lastStartedAt` existed for a
  doser use case (turn a pump on for a precisely-measured pulse to dispense an exact
  volume), sampled once per **minute** so a boundary stayed "true" for a full 60s without
  a guard. Not needed here: `Interval` mode is a stateless duty-cycle read
  (`isSuitableInterval`), not a triggered pulse, so there is no "did we already fire this
  boundary" state to protect at all.
- **Per-switch AND-condition dependencies** (ReefDuino's `std::vector<ScheduledSwitchDepsRule>`)
  — implemented, not dropped; see the `AutoSwitchDevice` section above. The device
  registry's existing support for repeated-role dependency links meant this was
  additive to `AutoSwitchDevice`'s dependency list rather than a redesign, as
  anticipated here previously.

## Key files

| Concern | File |
|---|---|
| Schedule config + codec | `src/devices/schedule/ScheduleDeviceConfig.{h,cpp}` |
| Schedule runtime | `src/devices/schedule/ScheduleDevice.{h,cpp}` |
| Schedule REST adapter | `src/integrations/rest/schedule/ScheduleDeviceApiAdapter.{h,cpp}` |
| Auto switch config + codec | `src/devices/switch/auto/AutoSwitchDeviceConfig.{h,cpp}` |
| Auto switch runtime | `src/devices/switch/auto/AutoSwitchDevice.{h,cpp}` |
| Auto switch REST adapter | `src/integrations/rest/auto_switch/AutoSwitchDeviceApiAdapter.{h,cpp}` |
| `setMode` command routing | `src/portal/controllers/DeviceRegistryController.cpp` (`command()`) |
| `DeviceRole::Schedule`/`Condition`, `IScheduleRuntime`/`IStatusRuntime`, `ProvidedRoles` | `src/devices/core/DeviceTypes.h` |
| Typed relationship validation (`providedRoles.contains(role)`) | `src/devices/registry/DeviceRegistrySnapshotValidator.cpp` |
| Current time + timezone | `src/time/DateTime.h`, `src/time/DateTimeTimezone.cpp` |
| Timezone applied on config write | `src/config/ConfigStore.cpp` (`save()`) |
| Frontend rule editor | `portal-spa/src/components/devices/schedule/ScheduleFields.vue` |
| Frontend auto switch card | `portal-spa/src/components/devices/auto-switch/AutoSwitchFields.vue` |
| Frontend device models | `portal-spa/src/models/devices/schedule.ts`, `auto-switch.ts` |
| Client-side schedule on/off preview | `portal-spa/src/models/devices/schedule-preview.ts` |
