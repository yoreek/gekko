---
title: Thermostat
description: How Gekko's thermostat keeps temperature at a setpoint — the control loop, hysteresis explained, and the safety rails around it.
sidebar:
  order: 7
---

## What it does

A thermostat closes the loop between a temperature sensor and a switch: *if
the water is too cold, turn the heater on; once it's warm enough, turn it
off*. In Gekko that's one `thermostat` device wired to two others:

![Control loop: DS18B20 measures, thermostat decides, relay drives the heater, water warms up, repeat](../../../../assets/diagrams/thermostat-loop.svg)

It works for cooling too — **cool** mode drives a chiller or a fan with the
same logic mirrored, and **off** parks the output.

## Hysteresis: why it doesn't flap

A naive "on below 25.0, off above 25.0" would chatter the relay dozens of
times a minute as the reading wiggles around the setpoint. The fix is a
**dead band** — the hysteresis:

![Hysteresis chart: heater on below 24.5, off at 25.0, nothing switches inside the band](../../../../assets/diagrams/thermostat-hysteresis.svg)

With target 25.0 °C and hysteresis 0.5 °C in heat mode:

- the heater turns **on** when the temperature falls to **24.5** (target −
  hysteresis);
- it stays on until the temperature reaches **25.0**, then turns **off**;
- anywhere in between, nothing switches — the temperature is left to drift
  across the band.

Bigger hysteresis = fewer relay cycles but wider temperature swing; smaller =
tighter control but more switching. For an aquarium heater, 0.3–0.5 °C is a
sensible range. On top of it, **min switch interval** (default 5 s) enforces a
hard floor between output flips — cheap insurance for relays, essential for
compressor-based chillers, which must not be short-cycled.

## Safety rails

The thermostat assumes things will occasionally go wrong and fails toward
"heater off":

- **Safe range** (`minSafeCelsius` / `maxSafeCelsius`) — a reading outside
  this window is treated as a fault (sensor fell out of the water, wiring
  broke to a fixed value): the output goes to its safe state and the status
  shows `out_of_range`.
- **Sensor timeout** — no fresh reading within `sensorTimeoutMs` (a dead bus,
  a disabled sensor) also stops heating: `sensor_timeout`.
- **Retry back-off** — after an error the thermostat waits
  `retryAfterErrorMs` before trying again instead of banging on a broken
  sensor every second.
- The **switch's own safe state** covers the reverse failure — if the
  thermostat itself is disabled or deleted, the
  [switch falls back](/gekko/reference/devices/gpio-switch/) to the state you
  configured there.

## Setting it up

1. Create the [DS18B20 sensor](/gekko/reference/devices/ds18b20/) (or NTC/HTU21).
2. Create the [switch](/gekko/reference/devices/gpio-switch/) driving the
   heater's relay. Heaters are a case where you should think about
   `safeState: off` and `startupState: off`.
3. Create the **thermostat**: pick the sensor and the switch, set mode,
   target, and hysteresis.

![Thermostat settings in the portal](../../../../assets/screenshots/device-thermostat.png)

## Configuration

| Field | Default | Meaning |
| --- | --- | --- |
| `mode` | `heat` | `heat`, `cool`, or `off` |
| `targetCelsius` | `25` | The setpoint |
| `hysteresisCelsius` | `0.5` | The dead band below (heat) or above (cool) the target |
| `minSafeCelsius` / `maxSafeCelsius` | `0` / `50` | Fault limits for the sensor reading |
| `checkIntervalMs` | `1000` | Control loop period |
| `sensorTimeoutMs` | `6000` | Max age of a reading before `sensor_timeout` |
| `minSwitchIntervalMs` | `5000` | Minimum time between output flips |
| `retryAfterErrorMs` | `30000` | Back-off before retrying after an error |

## Runtime & Home Assistant

The runtime reports the current temperature, the output state, and a status —
`heating`, `cooling`, `idle`, `sensor_timeout`, `out_of_range`,
`dependency_blocked` — surfaced with icons in the portal and recorded in the
device event journal. On [MQTT builds](/gekko/guides/mqtt-home-assistant/) the
thermostat appears in Home Assistant as a full `climate` entity (mode,
setpoint, current temperature, action), and setpoint changes from HA are
validated against the safe range.
