---
title: Pixel strip (WS2812B)
description: Addressable WS2812B RGB strips in Gekko — a hardware backend plus solid-color and alert-blink effects, both controllable live and from Home Assistant.
sidebar:
  order: 13
---

## The building blocks

An addressable strip is more than an on/off or dimmable output — it's an array
of colors. Gekko models it with one hardware backend and effect devices that
target it, the same decorator pattern as [analog outputs](/gekko/reference/devices/analog-outputs/):

| Type | What it does |
| --- | --- |
| `pixel_strip` | One WS2812B data pin — owns the pixel buffer and writes it to the hardware |
| `pixel_effect_solid` | Fills the target strip with one static color |
| `pixel_effect_alert` | Blinks the target strip while its conditions are satisfied |

Each effect takes exactly one `pixel_strip` dependency and holds it
**exclusively** — you can't wire two effects to the same strip at once, so
they never fight over it. Effects don't chain onto each other yet (unlike
fade/scheduled analog outputs); each strip runs one effect at a time.

## `pixel_strip`

The hardware device. Configuration:

- **Pin** — the GPIO wired to the strip's data-in line.
- **Pixel count** — how many LEDs are on the strip (up to 300).
- **Startup brightness** — the brightness applied on boot when there's no
  retained state to restore.
- **Restore previous state** — power up at the last live brightness instead
  of always starting at the configured startup value.

Brightness and on/off are **live state**, not part of the saved config —
dragging the dashboard slider or flipping the device off never marks the
config as changed or prompts a save dialog. Turning it off always shows
black at the hardware; turning it back on restores whatever brightness was
last set, so you never have to re-enter a value.

## `pixel_effect_solid`

Fills its target strip with one color and holds it — the simplest way to
just light a strip a single hue (a moonlight channel, an accent light, a
static reef white).

- **Color** picker sets the live color directly from the widget; the config
  form's color picker only sets the **startup color** applied on boot.
- **Restore previous state** works exactly like `pixel_strip`'s: restore the
  last live color, or always start from the startup color.
- Same explicit on/off gate as `pixel_strip` — off is always black at the
  hardware regardless of the configured color, independent of whatever color
  is stored.

## `pixel_effect_alert`

Blinks its target strip between a configured **color** and black at a
configured **blink interval**, while a bounded list of up to 4 `Condition`-role
devices (a schedule, a switch, an auto-switch, …) are all satisfied — the
same AND-condition mechanism [`auto_switch`](/gekko/guides/schedules-and-automation/)
uses. An empty condition list is never satisfied, so a misconfigured alert
can't blink by accident. Unlike `pixel_strip`/`pixel_effect_solid`, color and
blink interval are plain persisted config here — an alert's color and cadence
describe what the alert *means*, not a value you'd tweak live.

Typical use: wire an overflow float switch or a leak `binary_sensor`'s
derived condition into a red alert strip near the tank.

## Runtime & control

`pixel_strip` reports its live brightness and pixel count; `pixel_effect_alert`
reports whether its conditions are currently satisfied. A dashboard slider or
color picker drives brightness/color directly.

On [MQTT builds](/gekko/guides/mqtt-home-assistant/), all three types are
discoverable in Home Assistant: `pixel_strip` and `pixel_effect_solid` each
publish as a `light` (brightness-only and RGB-only, respectively), and
`pixel_effect_alert` publishes as a `binary_sensor`. Internals:
[`docs/pixel-strip.md`](https://github.com/yoreek/gekko/blob/master/docs/pixel-strip.md).
