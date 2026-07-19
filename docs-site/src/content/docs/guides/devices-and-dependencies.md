---
title: Devices & dependencies
description: How Gekko's typed device registry and dependency graph work.
sidebar:
  order: 1
---

The core idea behind Gekko is a **device registry**: a persisted list of device
instances, each created from one of the built-in
[device types](/gekko/reference/devices/), with its own configuration and live
runtime state.

## Devices are composed, not configured in isolation

Real hardware is layered — a sensor sits on a bus, a switch sits behind a port
expander, an automation drives a switch. Gekko models this directly: a device
**declares dependencies** on other devices, by role. Examples:

| This device… | …depends on |
| --- | --- |
| DS18B20 temperature probe | a 1-Wire bus device (which owns the GPIO) |
| SSD1306 OLED display | an I2C bus device |
| Switch on a PCF8574 | the port expander device |
| Thermostat | a temperature sensor **and** a switch |
| Auto switch | a real switch, plus up to 6 condition devices |
| Scheduled analog output | an analog output channel |

The registry validates the graph when you create or edit a device — you cannot
attach a display to a device that is not an I2C bus, and you cannot delete a
bus while a sensor still depends on it. Dependencies are picked in the portal's
device dialogs from lists already filtered to compatible devices.

## Roles, not hardcoded pairs

Dependencies are matched by **role** (`switch`, `temperature_sensor`,
`i2c_bus`, `condition`, …), and a device type can provide several roles. A GPIO
switch is both a `switch` and a `condition`, so an auto switch can use it
either as the output it drives or as an input condition. An auto switch itself
provides `switch` and `condition` too, so automations can be chained.

## Config vs. runtime state

Every device separates:

- **Config** — persisted settings (name, pins, rules, dependencies). Stored on
  the device in versioned binary form and migrated automatically across
  firmware upgrades. This is what [backup bundles](/gekko/guides/backup-restore/)
  contain.
- **Runtime** — live state (on/off, temperature, status such as `ready` or
  `dependency_blocked`). Never persisted inside config; streamed to the portal
  over WebSocket in real time.

A few types additionally keep small **retained state** across reboots — for
example a switch's last output state (when "restore previous state" is on) or
an auto switch's paused countdown — without rewriting their config.

## Lifecycle

Devices can be **enabled/disabled** without deleting them, and every instance
reports a status the portal surfaces: a sensor with a missing bus shows
`dependency_blocked`, a faulted device shows its error, and the
**Device events** journal records the transitions.
