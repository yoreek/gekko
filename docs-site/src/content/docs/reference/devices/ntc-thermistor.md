---
title: NTC thermistor temperature sensor
description: Reading temperature from a cheap NTC thermistor in Gekko — the voltage divider, presets, the Beta and Steinhart-Hart curves, and calibration.
sidebar:
  order: 12
---

## What is an NTC thermistor?

An NTC thermistor is a resistor whose resistance **drops as it gets hotter**
(NTC = Negative Temperature Coefficient). They're the cheapest temperature
sensor there is — a few cents — and come in glassbead, epoxy, and waterproof
probe forms. The catch versus a [DS18B20](/gekko/reference/devices/ds18b20/) is
that a thermistor is *analog*: it just changes resistance, so you need to
measure that resistance and convert it to a temperature. Gekko does both.

Compared to a DS18B20, an NTC is cheaper and can be physically tiny or very fast
to respond, but it's less accurate out of the box, needs a resistor and an ADC,
and cable resistance can nudge the reading. Use a DS18B20 when you want
plug-and-play accuracy; use an NTC when you want cheap, small, or fast — or when
you've already got an [ADS1115](/gekko/reference/devices/analog-inputs/) with a
spare channel.

## Wiring: the voltage divider

You can't read resistance directly — you read a voltage. So the thermistor goes
in series with a fixed **series resistor** to form a divider between the supply
and ground, and Gekko measures the voltage at the midpoint:

![NTC voltage divider feeding an analog input, then the NTC sensor converting millivolts to a temperature](../../../../assets/diagrams/ntc-divider.svg)

As the NTC's resistance changes with temperature, the midpoint voltage moves;
Gekko converts that voltage back to the NTC's resistance (it knows the series
resistor and the supply), then resistance to temperature. A **10 kΩ** series
resistor paired with a **10 kΩ (at 25 °C)** thermistor is the classic
combination and Gekko's default.

That midpoint is just an analog voltage — so the NTC sensor doesn't own an ADC
pin at all. It depends on an **[analog input](/gekko/reference/devices/analog-inputs/)**,
which means you can wire the divider into:

- the **ESP32's own ADC pin** (`analog_port_input`) — simplest, least precise;
- an **ADS1115 channel** (`analog_input_channel` on an `ads1115_hub`) — the
  precise option, and what makes a cheap thermistor genuinely usable;
- a **CD74HC4067 channel** — when you have many thermistors sharing one ADC pin.

## Setting it up

1. Create the analog input the divider midpoint is wired to — see
   [Analog inputs](/gekko/reference/devices/analog-inputs/). An ADS1115 channel
   is the recommended choice for a stable reading.
2. Create an **`ntc_thermistor_temperature_sensor`** and select that analog
   input as its dependency.
3. Pick a **preset** matching your thermistor, or enter the numbers by hand.

![NTC sensor settings: analog input picker, preset, divider values, formula mode and reporting](../../../../assets/screenshots/device-ntc-thermistor.png)

### Presets are just a shortcut

The form offers a handful of common thermistor models:

| Preset | Series R | Nominal R (25 °C) | Beta |
| --- | --- | --- | --- |
| Generic 10k B3950 | 10 kΩ | 10 kΩ | 3950 |
| EPCOS/TDK 10k B3435 | 10 kΩ | 10 kΩ | 3435 |
| Vishay 10k B3977 | 10 kΩ | 10 kΩ | 3977 |
| Semitec 100k B4267 | 100 kΩ | 100 kΩ | 4267 |

A preset only **pre-fills the numeric fields** — nothing about the choice is
stored on the device. Selecting one and then tweaking a value afterward is
always safe; the preset never "fights" your edits. Pick the closest one and
adjust, or select *Custom* and enter your thermistor's datasheet values.

## The two curves: Beta vs Steinhart-Hart

Turning resistance into temperature needs a model of the thermistor's curve.
Gekko offers both standard ones:

- **Beta equation** — `1/T = 1/T₀ + (1/β)·ln(R/R₀)`. The two-point form every
  datasheet publishes: nominal resistance R₀ at nominal temperature T₀ (usually
  10 kΩ at 25 °C) plus a single **Beta** coefficient. Accurate to roughly
  ±0.5–1 °C across an aquarium range — plenty for a heater or a chiller. This is
  the default and the easiest to fill in.
- **Steinhart-Hart equation** — `1/T = A + B·ln(R) + C·ln(R)³`. Three
  coefficients instead of one, more accurate over a wider range when you know
  them (or fit them from a 3-point resistance/temperature table). Choose this
  only if you have the A/B/C values; otherwise Beta is the right call.

Switch between them with the **formula mode** selector; the form shows the
fields the chosen equation needs.

## Calibration and smoothing

Because a thermistor reading depends on tolerances (the thermistor's own, the
series resistor's, the ADC's), the sensor shares Gekko's standard sensor
conditioning:

- a **calibration offset/factor** to trim a known error against a reference
  thermometer;
- a **smoothing weight** to damp the last bit of ADC jitter.

Drop a reference thermometer next to the probe, read both, and nudge the offset
until they agree — that single-point trim removes most of a cheap thermistor's
error.

## Watching it

The sensor reports its temperature with a validity flag — if its analog input
goes invalid (a disconnected divider, a sick I2C bus behind an ADS1115), the
reading shows as *invalid*, never as a stale or fake number. Click its dashboard
tile for the live value and a history chart, exactly like the DS18B20.

The temperature feeds everything else in Gekko the same way any temperature
sensor does:

- a [thermostat](/gekko/reference/devices/thermostat/) controlling a heater or
  chiller;
- [display placeholders](/gekko/guides/displays/) on any supported display;
- Home Assistant as a read-only `sensor` entity on
  [MQTT builds](/gekko/guides/mqtt-home-assistant/).

## Configuration

| Field | Default | Meaning |
| --- | --- | --- |
| `formulaMode` | `beta` | `beta` or `steinhart_hart` |
| `seriesResistorOhms` | `10000` | The fixed divider resistor, in ohms |
| `supplyMilliVolts` | `3300` | Divider supply voltage (3.3 V rail) |
| `nominalResistanceOhms` | `10000` | Thermistor resistance at the nominal temperature (R₀) |
| `nominalTempCelsius` | `25` | The nominal temperature (T₀) |
| `betaCoefficient` | `3950` | Beta value (Beta mode) |
| `steinhartA` / `steinhartB` / `steinhartC` | `0` | Steinhart-Hart coefficients (Steinhart-Hart mode) |
| `unit` | `celsius` | Display unit |
| `pollMs` | `5000` | How often to read |
| `reportDeltaCelsius` | `0.1` | Minimum change before a new reading is pushed |
| `reportAlways` | off | Push every poll regardless of the delta |

Temperature moves slowly — the default 5 s poll with a small report delta keeps
the WebSocket and history quiet without missing anything real.

Firmware internals, curve maths, and the preset table:
[`docs/analog-input.md`](https://github.com/yoreek/gekko/blob/master/docs/analog-input.md).
