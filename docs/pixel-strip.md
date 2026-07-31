# Pixel Strip (WS2812B)

Three device types model one addressable RGB LED strip and let effects be composed on
top of it without leaving the registry's existing dependency-graph model:
`pixel_strip` (`PixelStripDevice`, the Adafruit NeoPixel hardware backend),
`pixel_effect_solid` (`PixelEffectSolidDevice`), and `pixel_effect_alert`
(`PixelEffectAlertDevice`). This mirrors the `analog_output` family
(`docs/analog-output.md`) — one hardware backend, decorator devices consuming it by
role — but is a **new, sibling role**, not a reuse of `AnalogOutput`: a strip is an
array of pixel colors, not one scalar `0..kAnalogOutputLevelMax`, so it does not fit the
"one output = one scalar" rule the analog-output family relies on.

## Why Adafruit NeoPixel, not WS2812FX

`kitesurfer1404/WS2812FX` was evaluated and rejected as the runtime engine. It bundles
pixel buffer, ~80 built-in effects, and its own timing into one object, and its
`service()` call times itself off `millis()` internally — even when the caller passes an
external `now` into its own wrapper, the library's *own* effect timing is not
externally injectable. That conflicts directly with this project's rule that
`tick(uint32_t now)` never calls `millis()` and must be drivable on injected time (see
CLAUDE.md's "Do not call `millis()` inside state or tick handlers" and the native-test
discipline every other device family already follows). `Adafruit_NeoPixel` has no such
problem: it is a thin buffer-plus-`show()` wrapper with no internal timing at all, so
every effect's timing lives in Gekko's own tick-driven decorator code, fully
deterministic and testable on native without any wall clock.

This tradeoff was deliberate, not an oversight: WS2812FX ships a much larger effect
library out of the box. Gekko's own decorator effects can still be inspired by (or port
the pixel math from) WS2812FX's algorithms — the point rejected is its object model
(monolithic buffer+effect+timing ownership), not its ideas.

## `pixel_strip` — hardware backend (`src/devices/pixel/`)

One physical data pin = one `pixel_strip` device. Owns:

- A bounded, static (no heap) `PixelColor buffer_[kMaxPixelStripLength]`
  (`kMaxPixelStripLength = 300`, `src/devices/core/DeviceTypes.h`) — decorators write
  into this buffer via `setPixel`/`fill`, which never touch hardware.
- An `Adafruit_NeoPixel` instance (default-constructed, then `updateLength`/`updateType`/
  `setPin`/`begin` in `configureHardware()`), guarded by
  `#if defined(ARDUINO) && !defined(UNIT_TEST)` exactly like `LedcAnalogOutputDevice`
  guards its `ledcSetup`/`ledcAttachPin` calls — so native tests exercise the buffer
  logic with zero hardware dependency.

`show(now)` copies the buffer into the `Adafruit_NeoPixel` object and calls `.show()`;
it is the only place a bus transaction happens. `pixel_strip` never animates on its own
tick (`ticks100ms = true` only for lifecycle bookkeeping — mirrors `I2cBusDevice`/
`OneWireBusDevice`/`SpiBusDevice`, all passive hardware owners that still tick at 100ms
for the same reason). Frame cadence belongs entirely to whichever effect decorator is
attached, exactly as `FadeAnalogOutputDevice` owns its own step cadence over the passive
`LedcAnalogOutputDevice`.

`IPixelStripRuntime` (`DeviceTypes.h`) is the role-marker interface:
`pixelCount()`/`setPixel()`/`fill()` (buffered) and `show(now)` (hardware write). No
`w`/white channel — WS2812B has none; a future RGBW chip family (SK6812) would be an
additive config version, not a field added here on speculation.

RMT/pin-conflict arbitration: none is added, deliberately. A code search confirmed no
resource-pool abstraction exists in the project even for the analogous LEDC-channel
case (`ledcChannel` is an unvalidated `uint8_t` field, two devices can silently claim
the same channel today). `pixel_strip`'s `pin` gets the same range-only validation every
other pin field gets — consistent with existing (if imperfect) project convention rather
than a special case.

### Brightness is live state, not persisted config

`brightness` is **not** a config field. Following the same split
`AbstractOutputDevice`/`OutputDeviceConfigV1` establish for `analog_output`
(`docs/analog-output.md`): config only holds what to power up with —
`startupBrightness` (0..100 percent at the REST boundary, stored internally as
`0..255` via `percentToPixelBrightness`/`pixelBrightnessToPercent`,
`PixelStripDeviceConfig.h`) and `restorePreviousState`. The value actually driving the
strip right now, `liveBrightness()`, lives in `PixelStripRetainedStateV1` and is saved
through `DeviceRetainedDataStore` — completely decoupled from the config revision, so
changing it never bumps `configRevision` or requires a config-edit dialog.

`handleCommand()` accepts `DeviceCommandType::SetOutput` with a bare `0..100` percent
payload (routed by `DeviceRegistryController::cmd()`'s `pixelStripRuntime() != nullptr`
branch, alongside the existing `analogOutputRuntime`/`switchOutputRuntime` branches),
applies it immediately (`applyLiveBrightness()` + `show(now)`), and marks retained state
dirty for the registry to flush. On boot, `initializeHardware()` picks `liveBrightness_`
from the retained record if `restorePreviousState` is true and a retained value exists,
otherwise from `startupBrightness` — mirroring `AbstractOutputDevice`'s startup-state
selection exactly. The SPA's `AnalogOutputLevelControl` slider on the widget (not the
config form) drives this via `{command:'setOutput', state: <0..100>}`; the config form's
slider only edits the config-side `startupBrightness`.

### `on`/`off` is an explicit gate, not derived from brightness

`liveOn()` is a second, independent live/retained field alongside `liveBrightness()` —
**not** computed as `liveBrightness() == 0`. An earlier revision of the HA integration
derived on/off that way (mirroring how `AnalogOutputHaEntityAdapter` still does it for
`analog_output`), but that meant dragging the brightness slider to 0% silently flipped an
unrelated HA toggle, and there was no way to store "off, but remember 70% for next time"
— turning back on always meant guessing/re-entering a brightness. `handleCommand()`
recognizes a second `SetOutput` payload shape, `{"on": bool}` (tried after the bare-percent
parse fails), which flips `liveOn_` without touching `liveBrightness_` at all.
`applyLiveBrightness()` is where the two combine: `strip_.setBrightness(liveOn_ ?
liveBrightness_ : 0)` — off always shows black at the hardware, on shows whatever
brightness was already set, independently of when it was last changed. `on` is retained
and restored exactly like `brightness` (same `restorePreviousState` flag, same
`PixelStripRetainedStateV1` record).

## `pixel_effect_solid` — static-color decorator

Fills the whole target strip with one `color` and holds it. Depends on exactly
one `PixelStrip`-role dependency, held **exclusively**
(`exclusiveDependencyRoles = ProvidedRoles::of({DeviceRole::PixelStrip})`) — the same
"only one controlling dependent per target" rule `FadeAnalogOutputDevice` relies on over
`analog_output`, so two effects can't fight over one physical strip. Provides no role
back (terminal decorator in v1, unlike Fade/Scheduled which re-provide `AnalogOutput` to
support chaining) — there is no pixel-effect chaining yet.

Like `pixel_strip`'s brightness, `color` is live retained state, not persisted config:
config holds `startupColor` + `restorePreviousState`; `liveColor()` is backed by
`PixelEffectSolidRetainedStateV1` via `DeviceRetainedDataStore`, set immediately by a
`SetOutput` command carrying a `{"r":..,"g":..,"b":..}` object payload
(`parseSetOutputColor()`), and restored from either the retained record or
`startupColor` on boot the same way `pixel_strip` restores brightness. The widget's
color picker edits this live value directly; the config form's color picker only edits
`startupColor`.

Same explicit `on`/`off` gate as `pixel_strip`, and for the same reason: `liveOn()` is a
second, independent live/retained field, **not** derived from `liveColor() == black`.
`handleCommand()` recognizes a second `SetOutput` payload shape, `{"on": bool}` (tried
after the `{"r":..,"g":..,"b":..}` parse fails), which flips `liveOn_` without touching
`liveColor_`. `applyColorIfNeeded()` combines them: it fills the target strip with
`liveOn_ ? liveColor_ : PixelColor{}` — off always shows black regardless of the
configured color, on shows whatever color was already set. Picking black in the color
picker is now indistinguishable from "off" only by coincidence of value, never by
accident of a derived comparison.

`pixel_effect_alert`'s `color`/`blinkIntervalMs` remain plain persisted config — an
alert's color and cadence are part of *what the alert means*, not a live control surface
someone adjusts moment-to-moment, so they were deliberately left out of this split.

## `pixel_effect_alert` — Condition-driven blink decorator

Blinks the target strip between `color` and black at `blinkIntervalMs` while a bounded,
ANDed list of `Condition`-role dependencies is satisfied (`kMaxPixelEffectAlertConditions
= 4`), holding steady black otherwise. This is the "indicate something" use case: wire
any existing `Condition`-role device (a schedule, a switch, an `AutoSwitchDevice`, or a
future dedicated alert-condition type) into it, exactly the same mechanism
`AutoSwitchDevice` already uses for its AND-condition list — no new dependency machinery
was introduced for this.

`conditionsSatisfied()` copies `AutoSwitchDevice::conditionsSatisfied()`'s semantics
exactly: an empty condition list is never satisfied, so a misconfigured alert can't
blink constantly by accident.

The blink implementation tracks whether the *previous* tick was satisfied
(`wasSatisfied_`) as well as the current tick, so an unsatisfied→satisfied transition
repaints immediately on its own on-phase and restarts the blink clock, instead of
resuming whatever was left of a timer that had been counting down for the unrelated
black/unsatisfied state. Within a steady state, `now - lastToggleAt_` /
`blinkIntervalMs` interval arithmetic (mirroring
`FadeAnalogOutputDevice::onReadyTick`'s elapsed/intervals math) decides whether the
phase should flip, and the strip is only repainted when the phase actually changes —
not on every 100ms tick.

## Adding a new effect later

A new pixel effect (rainbow, chase, breathe, fire, ...) is a new device type depending
on the `PixelStrip` role, following `pixel_effect_solid`/`pixel_effect_alert` as
templates — it never requires touching `pixel_strip` itself or the `IPixelStripRuntime`
contract. See `docs/device-scaffolding.md` for the full manual checklist (config struct,
runtime class, descriptor registration, REST adapter, SPA model/UI/mock seed/i18n).

## REST API

See `docs/rest-api-contract.md` for the public config/runtime JSON shape. `pixel_strip`'s
`runtime.output` reports only `pixelCount`/`brightness`, deliberately not the per-pixel
buffer, to keep the websocket payload bounded (mirrors why
`FadeAnalogOutputDeviceApiAdapter::writeRuntimeJson` writes a summary rather than raw
state). `pixel_effect_alert`'s `runtime.output.active` reports whether its condition list
is currently satisfied.
