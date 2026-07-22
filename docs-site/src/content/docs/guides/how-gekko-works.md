---
title: How Gekko works
description: Understand the physical wiring, device dependencies, and runtime control flow before creating devices.
sidebar:
  order: 0
---

Gekko turns connected hardware into a graph of small, composable devices. The
same system is easiest to understand from three views: what is wired to the
ESP32, which Gekko devices depend on each other, and how readings become
actions at runtime.

## 1. Physical wiring

Hardware connects to ESP32 pins and buses. A temperature-control project, for
example, has a DS18B20 probe on a 1-Wire data pin and a relay on an output pin.
The heater is connected to the relay, not directly to the ESP32.

Physical wiring answers: **which pins, buses, modules, and loads are present?**

## 2. Device dependencies

In Gekko, each hardware or control role is a device instance. A sensor depends
on the bus that communicates with it; a thermostat depends on both a compatible
temperature sensor and switch.

![Temperature-control device graph: a 1-Wire bus provides the DS18B20 sensor; the sensor and GPIO switch are dependencies of the thermostat.](../../../assets/diagrams/thermostat-project-flow.svg)

Dependencies answer: **what must exist first, and what becomes unavailable if
another device fails?** Gekko validates compatible roles when you create or edit
a device. A dependent device reports `dependency_blocked` rather than acting on
stale or invented values.

## 3. Runtime control flow

At runtime, values and commands move through the graph:

```text
DS18B20 temperature → thermostat decision → On/Off command → relay → heater
```

The thermostat reads the temperature, compares it with the configured target
and hysteresis, and commands the switch. The switch drives the relay; the relay
switches the heater. This is a control flow, not a second physical wire.

Runtime flow answers: **what information moves, who decides, and what output is
affected?**

## Build in dependency order

For any project, create infrastructure before the devices that use it:

1. Create buses and hardware outputs.
2. Create sensors or channels that depend on them.
3. Verify each device reaches `ready`.
4. Create control or automation devices that combine those inputs and outputs.
5. Test normal operation and the safe behaviour when a dependency is missing.

Use [Devices & dependencies](/gekko/guides/devices-and-dependencies/) for the
underlying registry rules, or start with a complete
[thermostat with relay project](/gekko/projects/thermostat-with-relay/).
