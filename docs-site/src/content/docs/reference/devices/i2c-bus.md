---
title: I2C bus
description: How the I2C bus works — two wires, 7-bit addresses, what runs on it in Gekko, and the built-in bus diagnostics.
sidebar:
  order: 4
  label: I2C bus
---

## What is I2C?

I2C (Inter-Integrated Circuit, «i-squared-C») is the workhorse two-wire bus of
hobby electronics: one **data** line (SDA) and one **clock** line (SCL),
shared by every device. Like [1-Wire](/gekko/reference/devices/onewire-bus/),
many devices coexist on the same wires — but here each chip has a short
**7-bit address**, usually printed in its datasheet (and often selectable with
solder jumpers).

In Gekko the bus is its own device, `i2c_bus`: it owns the two pins, runs the
address scan, and every I2C peripheral you add declares a dependency on it.

## Wiring

![I2C wiring: SDA and SCL with pull-ups, OLED, HTU21, DS3231 and PCF8574 in parallel, each with its address](../../../../assets/diagrams/i2c-wiring.svg)

Both lines are open-drain and need pull-up resistors to 3.3 V. In practice
you rarely add them yourself: **virtually every breakout module (OLED, RTC,
HTU21, expander) already has pull-ups on board**, and the `i2c_bus` device
enables the ESP32's internal pull-ups by default. Only a bare chip on a long
line needs explicit resistors (2.2–10 kΩ).

Devices connect in parallel: SDA to SDA, SCL to SCL, plus 3.3 V and GND. Keep
the wires reasonably short (tens of centimetres at the default speed) — I2C
is a board-level bus, not a cable bus like 1-Wire.

## Who lives on the I2C bus in Gekko

| Device | Typical address |
| --- | --- |
| SSD1306 OLED display | `0x3C` (sometimes `0x3D`) |
| AHT10 temperature + humidity | `0x38` |
| HTU21 temperature + humidity | `0x40` |
| DS3231 real-time clock | `0x68` |
| PCF8574 / PCF8575 port expanders | `0x20`–`0x27` (jumper-selectable) |

Each of these is created as its own device that depends on the bus, with its
address in its own config. Two identical chips (e.g. two PCF8574s on different
jumper addresses) are simply two devices on the same bus — Gekko rejects
creating two devices with the same address on one bus.

## Scan and diagnostics

The device page has a **Scan bus** button — it probes all valid addresses and
lists everything that answers, which is the quickest way to confirm wiring and
find a module's actual address. Below it live the **bus diagnostics**:
consecutive error counts, the last error code, and the transaction state, with
a reset button. A wiring problem shows up here first — sensors on a sick bus
report `dependency_blocked` rather than fake values.

![I2C bus settings with scan and diagnostics](../../../../assets/screenshots/device-i2c-bus.png)

## Configuration

| Field | Default | Meaning |
| --- | --- | --- |
| `sdaPin` | `21` | Data line (ESP32's conventional I2C pins are 21/22) |
| `sclPin` | `22` | Clock line |
| `frequencyHz` | `100000` | Bus speed, 1–400 000 Hz; 100 kHz is the safe default, 400 kHz works with short wiring |
| `internalPullup` | on | Use the ESP32's internal pull-ups (fine alongside module pull-ups) |
| `enabled` | on | Disabling the bus blocks every device on it |

## Troubleshooting

- **Scan finds nothing** — swapped SDA/SCL is the classic; also check 3.3 V
  and GND to the module.
- **Device found at a different address** — jumpers (expanders) or a `0x3D`
  OLED variant; use the scanned address.
- **Errors under load / long wires** — drop `frequencyHz` back to 100 kHz,
  shorten the wiring, and keep display cables away from relay/mains wiring.
