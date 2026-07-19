---
title: GPIO switch
description: Reference for Gekko's gpio_switch device type — an on/off output on an ESP32 GPIO pin.
sidebar:
  order: 2
---

`gpio_switch` drives one GPIO pin as an on/off output — relay boards, MOSFET
modules, status LEDs. It is usually the first device you create; the
[first device walkthrough](/gekko/getting-started/first-device/) uses it.

![GPIO switch settings](../../../../assets/screenshots/device-gpio-switch.png)

## Dependencies

None — it owns its GPIO pin directly. (For outputs behind a PCF8574/PCF8575
expander, use `port_expander_switch` instead; it offers the same switch options
below.)

## Configuration

| Field | Default | Meaning |
| --- | --- | --- |
| `gpioPin` | `4` | The output pin |
| `inverted` | off | Invert the electrical level — enable for active-low relay boards |
| `startupState` | off | Output state right after boot (when not restoring) |
| `restorePreviousState` | off | Restore the last state from before the reboot instead of `startupState` |
| `safeState` | off | State to fall back to when a controlling device becomes unavailable |
| `enabled` | on | Disabled devices release their output and stop reporting |

## Runtime & control

The device reports its live on/off state; toggle it from the devices list, a
dashboard switch widget, the REST API, or Home Assistant (as a `switch` entity
on [MQTT builds](/gekko/guides/mqtt-home-assistant/)).

## Provides

- **switch** — can be driven by a thermostat, auto switch, or dosing pump.
- **condition** — its on/off state can gate an auto switch.
