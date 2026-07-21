---
title: HTU21 temperature & humidity sensor
description: The HTU21 I2C temperature and humidity sensor in Gekko — one small board reporting both air temperature and relative humidity.
sidebar:
  order: 6.5
---

## What is an HTU21?

The HTU21 (and its near-identical siblings SHT21 / Si7021) is a tiny I2C sensor
that reports **two** things at once: **air temperature** (±0.3 °C) and
**relative humidity** (±2 % RH). It comes as a small breakout the size of a
fingernail, which makes it the go-to for the *air* around a setup rather than
the water in it: room climate above an aquarium, humidity in a terrarium or
vivarium, the air in a grow tent or incubator.

Where a [DS18B20](/gekko/reference/devices/ds18b20/) is a waterproof probe on a
cable for *water* temperature, the HTU21 is a board-mounted sensor for *air* —
and it adds humidity, which the DS18B20 can't measure at all.

## Wiring: it's an I2C device

The HTU21 lives on the [I2C bus](/gekko/reference/devices/i2c-bus/) like any
other I2C peripheral — SDA, SCL, 3.3 V, GND, with the pull-ups almost always
already on the breakout. Its address is fixed at **`0x40`** (no jumpers), so you
can only have **one** HTU21 per bus; a second air sensor needs a second I2C bus
on different pins.

## Setting it up

1. Create an **[I2C bus](/gekko/reference/devices/i2c-bus/)** on your SDA/SCL
   pins (if you don't have one), and use **Scan bus** to confirm the sensor
   answers at `0x40`.
2. Create an **`htu21`** device and select that bus as its dependency.

![HTU21 settings: I2C bus picker, address, unit, and reporting deltas](../../../../assets/screenshots/device-htu21.png)

That's it — there's nothing to calibrate to get started. The device immediately
reports temperature and humidity, each with its own validity flag: a
disconnected sensor or a sick bus shows as *invalid*, never as a stale number.

## Two readings from one device

Unlike most sensors, an HTU21 produces two live values:

- **Temperature** — provides the `temperature_sensor` role, so it can drive a
  [thermostat](/gekko/reference/devices/thermostat/) (e.g. a terrarium heat mat),
  feed [display placeholders](/gekko/guides/displays/), and appear in Home
  Assistant.
- **Humidity** — reported as a percentage for the dashboard, displays, and Home
  Assistant.

On [MQTT builds](/gekko/guides/mqtt-home-assistant/) both show up in Home
Assistant — a `temperature` sensor and a `humidity` sensor — from the one
device.

## Calibration

Because temperature and humidity are independent measurements, each has its own
conditioning — a calibration offset/factor to trim against a reference, and a
smoothing weight to damp jitter. Trimming the humidity reading (against a
calibrated hygrometer or a salt-test reference) doesn't touch the temperature,
and vice versa.

## Configuration

| Field | Default | Meaning |
| --- | --- | --- |
| `i2cAddress` | `0x40` | Fixed HTU21 address — normally leave as is |
| `unit` | `celsius` | Temperature display unit |
| `pollMs` | `5000` | How often to read the sensor |
| `reportDeltaCelsius` | `0.1` | Minimum temperature change before a new reading is pushed |
| `reportDeltaHumidity` | `0.1` | Minimum humidity change before a new reading is pushed |
| `reportAlways` | off | Push every poll regardless of the deltas |
| `enabled` | on | Disabled devices stop reporting |

Air temperature and humidity drift slowly — the default 5 s poll with small
report deltas keeps the WebSocket and history charts quiet without missing
anything real.

## Provides

- **temperature_sensor** — its temperature can drive a thermostat or gate an
  auto switch.
