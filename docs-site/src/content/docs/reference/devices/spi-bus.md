---
title: SPI bus
description: How the SPI bus works — shared clock and data lines, chip-select per device, and driving the ST7735 TFT in Gekko.
sidebar:
  order: 5
  label: SPI bus
---

## What is SPI?

SPI (Serial Peripheral Interface) is the fast serial bus — where
[I2C](/gekko/reference/devices/i2c-bus/) tops out at 400 kHz, SPI happily
clocks tens of MHz, which is why color displays use it. The trade-off is more
wires: a shared **clock** (SCK) and **data** (MOSI, optionally MISO for data
coming back), plus an individual **chip-select** (CS) line per device — that's
how devices are told apart, instead of addresses.

In Gekko the bus is the `spi_bus` device: it owns the shared SCK/MOSI/MISO
pins and the ESP32 SPI host. Today its one consumer is the
**ST7735 color TFT** display; the display's own CS/DC/reset pins belong to the
display device, not the bus.

## Wiring

![SPI wiring: shared SCK and MOSI to the ST7735, chip-select and data/command pins per display](../../../../assets/diagrams/spi-wiring.svg)

A typical ST7735 module maps like this (names on the module silkscreen vary):

| Module pin | ESP32 pin (defaults) | Belongs to |
| --- | --- | --- |
| SCK / CLK | GPIO 18 | `spi_bus` |
| SDA / MOSI / DIN | GPIO 23 | `spi_bus` |
| CS | GPIO 5 | `st7735` device |
| DC / A0 | GPIO 2 | `st7735` device |
| RES / RST | — (or any free GPIO) | `st7735` device |
| VCC, GND, LED/BLK | 3.3 V / GND | power |

Displays never send data back, so **MISO stays at −1** (unused). A second SPI
device would share SCK/MOSI and simply get its own CS pin.

## Bus device and diagnostics

Like the other buses, `spi_bus` shows live diagnostics — error counters and
the transaction state — and disabling it blocks the display with
`dependency_blocked` instead of leaving stale pixels.

![SPI bus settings with diagnostics](../../../../assets/screenshots/device-spi-bus.png)

## Configuration

| Field | Default | Meaning |
| --- | --- | --- |
| `host` | `VSPI` | Which ESP32 hardware SPI controller to use |
| `sckPin` | `18` | Shared clock |
| `mosiPin` | `23` | Shared data out |
| `misoPin` | `-1` | Data in — unused for displays, set only if a future device needs readback |
| `enabled` | on | Disabling the bus blocks every device on it |

## Next step

Create the bus, then add an [ST7735 display](/gekko/guides/displays/) on it and
open the visual layout designer — pages, widgets, and live metric placeholders
work the same as on the OLED.
