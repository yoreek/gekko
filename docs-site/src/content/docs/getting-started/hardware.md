---
title: Hardware requirements
description: What you need to run Gekko — a plain ESP32 dev board with 4 MB of flash.
sidebar:
  order: 2
---

## The controller board

Gekko targets the classic **ESP32** (the original dual-core chip) with **4 MB of
flash** — the standard "ESP32 DevKit" style development board that usually costs
a few dollars. That is the configuration the prebuilt binaries are built for:

| Partition | Flash offset |
| --- | --- |
| Bootloader | `0x1000` |
| Partition table | `0x8000` |
| Firmware (single app, no OTA) | `0x10000` |
| LittleFS (web portal assets) | `0x370000` |

The default build uses a single-app layout without an OTA slot to fit the
firmware, the web portal, and your device configuration into 4 MB. Boards with
more flash work too and leave headroom for the optional
[Web OTA build](/gekko/guides/ota-updates/).

You will also need a **USB data cable** and, on some boards, the usual
CP210x/CH340 USB-serial driver for your operating system.

## Peripherals from the device catalog

Everything below is optional — you add each one from the web portal when you
actually wire it up:

- **Relays / MOSFET boards** on any free GPIO (`gpio_switch`)
- **PCF8574 / PCF8575** I2C port expanders for more switch outputs
- **DS18B20** waterproof temperature probes on a 1-Wire bus (one GPIO, many
  probes)
- **NTC thermistors** on an ADC pin
- **HTU21** I2C temperature + humidity sensor
- **SSD1306** I2C OLED displays and **ST7735** SPI TFT displays
- **DS3231** I2C real-time clock — recommended if you use schedules and the
  device may run without internet/NTP
- **Peristaltic dosing pumps** driven through a switch output
- **LED drivers / PWM loads** on LEDC-capable pins (`analog_output`)
- Digital inputs: float switches, door contacts, leak sensors
  (`binary_sensor`)

See the [device catalog](/gekko/reference/devices/) for the full list of the 23
built-in device types.

:::tip
Start with just the bare board. Flash it, connect it to WiFi, and click around
the portal — you can add real hardware one device at a time later.
:::
