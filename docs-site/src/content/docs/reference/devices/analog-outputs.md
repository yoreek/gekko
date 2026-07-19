---
title: Analog outputs & light composer
description: Dimmable PWM outputs in Gekko — smooth fades, daily brightness curves, and multi-channel fixtures like a five-channel aquarium light.
sidebar:
  order: 8
---

## Why dimmable outputs?

An on/off switch is fine for a heater — but a light shouldn't slam from 0 to
100% at 9 in the morning. A good aquarium (or terrarium, or greenhouse) light:

- **ramps up and down gradually** — sunrise and sunset, not a light switch;
  fish visibly startle at hard transitions, corals don't appreciate them
  either;
- **changes intensity over the day** — a midday peak, gentler mornings and
  evenings;
- **mixes several color channels** — reef fixtures typically have separate
  royal blue, blue, white, violet, and moonlight LED strings, each with its own
  daily curve.

Gekko models this with four device types that snap together like blocks. Every
level is a percentage (0–100%) in the portal and API; the hardware side is an
ESP32 PWM (LEDC) pin driving an LED driver's dimming input, a MOSFET module, or
any other PWM-controlled load.

## The building blocks

| Type | What it does |
| --- | --- |
| `analog_output` | The hardware PWM channel on a pin |
| `fade_analog_output` | Smooths every change into a gradual ramp |
| `scheduled_analog_output` | Drives its target along a daily level curve |
| `analog_output_composer` | Groups several channels into one fixture |

A fade or scheduled output takes exactly one `analog_output`-role dependency
and *provides the same role itself*, so they stack:

![Decorator chain: scheduled output computes the level, fade smooths it, analog output writes PWM](../../../../assets/diagrams/analog-chain.svg)

- **Fade** — `maxStep` (percent per step) and `stepIntervalMs` set the ramp
  speed; the default ≈1% per 200 ms turns any change, including a manual
  slider move, into a gentle transition.
- **Scheduled** — up to 10 `(time, level)` points per day, interpolated
  between points. Modes: **Off**, **Manual** (a fixed level), **Scheduled**
  (follow the curve). With no valid clock the output resolves to zero rather
  than holding a stale level.

The registry enforces that each output has **at most one controller** — you
cannot accidentally wire two schedules to the same channel.

## Worked example: a five-channel aquarium light

The goal — a day that looks like this:

![Daily curves of five channels: blues ramp first and linger, white peaks midday, violet accents, moonlight at night](../../../../assets/diagrams/aquarium-light-day.svg)

Blues come up first and fade out last (corals photosynthesize mostly in blue),
warm white fills the midday hours, violet adds fluorescence pop, and a faint
moonlight channel glows at night. To build it:

1. Create five **`analog_output`** devices, one per LED driver pin: "Royal
   blue LEDC", "Blue LEDC", "White LEDC", "Violet LEDC", "Moonlight LEDC".
2. Wrap each in a **`fade_analog_output`** ("Royal blue fade" → targets "Royal
   blue LEDC", …) so channel changes never jump.
3. Wrap each fade in a **`scheduled_analog_output`** ("Royal blue schedule" →
   targets "Royal blue fade", …) and draw that channel's daily curve.
4. Create one **`analog_output_composer`** "Aquarium light" and add the five
   scheduled outputs as its channels.

![Aquarium light composer in the portal](../../../../assets/screenshots/device-analog-composer.png)

The composer now behaves as *the* light:

- **One mode for the whole fixture** — switching the composer between Off /
  Manual / Scheduled pushes that mode to every channel and keeps them in sync
  if one ever diverges. Off zeroes everything.
- **One editor** — all channel curves on a single graph, edited in place by
  dragging points (right-click to insert/delete, optional 15-min/5% snapping,
  a sunrise/sunset underlay for reference); manual mode shows one slider per
  channel.
- **One dashboard card** — pin the composer for a compact multi-channel
  schedule preview.

The composer is only needed when several channels should act as one fixture —
a single-channel light is just steps 1–3 with one chain. Skip the fade layer if
you don't care about ramps.

## Runtime & control

All four types report their live level (fades also report the target and
whether they are still transitioning). A dashboard slider or the `setOutput`
command drives a channel directly; mode changes go through `setMode`. On
[MQTT builds](/gekko/guides/mqtt-home-assistant/) channels are discoverable in
Home Assistant. Internals:
[`docs/analog-output.md`](https://github.com/yoreek/gekko/blob/master/docs/analog-output.md).
