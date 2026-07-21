---
title: Port expanders (PCF8574 / PCF8575)
description: Adding more switch outputs over I2C in Gekko — the PCF8574 and PCF8575 port expanders and the port_expander_switch that drives one of their pins.
sidebar:
  order: 2.5
---

## Why a port expander?

The ESP32 has plenty of GPIO, but a busy tank can still run out — every relay,
every MOSFET wants a pin, and some pins are spoken for (the I2C bus, an SPI
display, the input-only ADC pins). A **port expander** is the cheap fix: an I2C
chip that gives you **8 or 16 extra I/O pins over the same two wires** the rest
of your I2C devices already share. Wire one expander and you've added a bank of
relay outputs without touching another ESP32 pin.

Gekko supports the two ubiquitous ones:

| Type | Chip | Extra pins | Addresses |
| --- | --- | --- | --- |
| `pcf8574_expander` | PCF8574 | 8 | `0x20`–`0x27` |
| `pcf8575_expander` | PCF8575 | 16 | `0x20`–`0x27` |

Both are `port_expander`-role hubs on an
[I2C bus](/gekko/reference/devices/i2c-bus/); the only difference is 8 vs 16
pins. Up to eight of each can share a bus, their addresses set by the A0/A1/A2
solder jumpers.

## Hub and channels

Just like the [ADS1115/multiplexer hubs](/gekko/reference/devices/analog-inputs/),
an expander is a **hub**: the expander device owns the chip, and each output pin
you actually use is a separate **`port_expander_switch`** device that depends on
it.

So a two-relay PCF8574 setup is three devices: the `pcf8574_expander`, and two
`port_expander_switch` devices (pin 0 and pin 1) pointing at it. Each switch is
independently named, enabled, and controllable — and behaves exactly like a
[GPIO switch](/gekko/reference/devices/gpio-switch/), just on an expander pin
instead of an ESP32 pin. Two switches can't claim the same pin on one expander;
Gekko rejects the second.

Because a `port_expander_switch` **provides the `switch` role**, anything that
drives a switch drives it too — a [thermostat](/gekko/reference/devices/thermostat/),
an `auto_switch`, a [dosing pump](/gekko/reference/devices/dosing-pump/). Nothing
in those controllers knows or cares that the output is behind an expander.

## Setting it up

1. Create an **[I2C bus](/gekko/reference/devices/i2c-bus/)** (if you don't have
   one) and use **Scan bus** to confirm the expander answers — usually at
   `0x20`.
2. Create a **`pcf8574_expander`** (or `pcf8575_expander`), select that bus, and
   set its address.
3. For each output, create a **`port_expander_switch`**, select the expander,
   and pick the pin number (0–7 on a PCF8574, 0–15 on a PCF8575).

![PCF8574 expander settings: I2C bus, address with scan, and the polarity option](../../../../assets/screenshots/device-pcf8574-expander.png)

Then the switch itself, with the same options as any GPIO switch:

![Port expander switch settings: expander picker, pin number, and switch options](../../../../assets/screenshots/device-port-expander-switch.png)

## Active-low relay boards

Most cheap relay boards are **active-low** — the relay closes when the pin is
pulled *low*, not high. There are two places to correct for that, and it's worth
being deliberate:

- **On the expander** — its `inverted` option flips the electrical polarity of
  the *whole chip*. Use this when the entire board is active-low.
- **On the switch** — its `inverted` option flips a *single* pin. Use this when
  only some pins are wired active-low.

Get this right and "on" in Gekko means the relay is actually energised.

## Configuration

### `pcf8574_expander` / `pcf8575_expander`

| Field | Default | Meaning |
| --- | --- | --- |
| `i2cAddress` | `0x20` | Chip address (`0x20`–`0x27` by the A0/A1/A2 jumpers) |
| `inverted` | off | Invert the electrical level of every pin (active-low boards) |
| `enabled` | on | Disabling the expander releases every switch on it |

### `port_expander_switch`

| Field | Default | Meaning |
| --- | --- | --- |
| `channel` | `0` | Which expander pin (0–7 on PCF8574, 0–15 on PCF8575) |
| `inverted` | off | Invert this one pin's electrical level |
| `startupState` | off | Output state right after boot (when not restoring) |
| `restorePreviousState` | off | Restore the last state from before the reboot instead of `startupState` |
| `safeState` | off | State to fall back to when a controlling device becomes unavailable |
| `enabled` | on | Disabled switches release their pin and stop reporting |

## Provides

A `port_expander_switch` provides the same roles as a GPIO switch:

- **switch** — can be driven by a thermostat, auto switch, or dosing pump;
- **condition** — its on/off state can gate an auto switch.

On [MQTT builds](/gekko/guides/mqtt-home-assistant/) each switch is discoverable
in Home Assistant as a `switch` entity. The expander itself isn't — it provides
pins, not a control of its own.
