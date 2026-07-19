---
title: 1-Wire bus
description: How the 1-Wire bus works — wiring, the pull-up resistor, 64-bit ROM addresses, and how many sensors share a single ESP32 pin.
sidebar:
  order: 3
  label: 1-Wire bus
---

## What is 1-Wire?

1-Wire is a serial bus (originally by Dallas Semiconductor) built around a
radical idea: **one shared data line for everything**. The controller and any
number of devices — in practice, DS18B20 temperature probes — all hang off the
same wire, and each device is addressed individually. That's why a whole
aquarium's worth of temperature probes costs you exactly **one GPIO pin**.

In Gekko, the bus itself is a device: `onewire_bus`. It owns the pin, runs the
scan, and every [DS18B20 sensor](/gekko/reference/devices/ds18b20/) you create
depends on it.

## Wiring: three wires and one resistor

![1-Wire wiring: 3V3, GND, DATA with a 4.7 kΩ pull-up between DATA and 3V3](../../../../assets/diagrams/onewire-wiring.svg)

The typical waterproof DS18B20 probe has three leads:

| Lead | Usual color | Connect to |
| --- | --- | --- |
| VDD | red | 3.3 V |
| DATA | yellow (or white/blue) | the bus GPIO |
| GND | black | GND |

The **4.7 kΩ pull-up resistor** between DATA and 3.3 V is not optional. The
data line is *open-drain*: no device ever drives it high — they only pull it
low and release it. The resistor is what returns the line to 3.3 V between
bits; without it every read is garbage. One resistor per bus, regardless of
how many probes are on it.

:::tip
The `onewire_bus` config has an **internal pull-up** toggle that uses the
ESP32's built-in ~45 kΩ resistor. It can work for one probe on a short lead,
but it is much weaker than the recommended 4.7 kΩ — for anything longer than
a breadboard, fit the real resistor.
:::

Probes sold as "with adapter/module" often have the resistor already on the
little board — don't add a second one.

## Many devices on one wire

![Bus topology: ESP32 with a pull-up and three probes on one data line, each with its own ROM address](../../../../assets/diagrams/onewire-bus.svg)

New probes are wired **in parallel** onto the same three lines — tap DATA,
3.3 V, and GND wherever convenient. A daisy-chain (probe to probe along one
cable) is electrically cleanest; short stubs off a main line are fine too.
Runs of several metres are routine with the 4.7 kΩ pull-up.

## Addresses: how probes avoid colliding

Every 1-Wire device is identified by a unique **64-bit ROM address**:

![64-bit ROM address anatomy: 8-bit family code, 48-bit unique serial, 8-bit CRC; scan discovers, match addresses one device](../../../../assets/diagrams/onewire-rom.svg)

Two operations make the shared wire work:

- **Search ("scan" in the portal)** — a clever binary elimination that
  discovers every address on the bus without knowing any of them in advance.
  Gekko's bus device exposes this as a **scan** command; the results (family
  code, address, CRC status) feed the DS18B20 creation dialog directly.
- **Match** — to talk to one device, the controller sends its full address
  first; only that device responds. Devices never speak unprompted, so there
  are no collisions — the controller always polls.

The first byte is the **family code** — `28` means "DS18B20 temperature
sensor". The scan reports everything it finds, and Gekko filters DS18B20
candidates by that code.

## Configuration

| Field | Default | Meaning |
| --- | --- | --- |
| `gpioPin` | `4` | The data pin of the bus |
| `internalPullup` | off | Use the ESP32's weak internal pull-up instead of an external 4.7 kΩ |
| `enabled` | on | Disabling the bus blocks every sensor on it (`dependency_blocked`) |

## Troubleshooting

- **Scan finds nothing** — check the pull-up first, then the wiring order
  (swapping VDD and DATA is the classic mistake; probes survive it).
- **Probe appears with a CRC flag** — bad contact or interference; shorten the
  line, improve joints, keep it away from mains cables and switching PSUs.
- **Two probes, one address shown** — you scanned before connecting the
  second probe; re-run the scan.
