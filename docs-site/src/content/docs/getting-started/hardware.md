---
title: Hardware requirements
description: What you need to run Gekko — an ESP32-family dev board with 4 MB of flash.
sidebar:
  order: 2
---

## The controller board

Gekko runs on five ESP32-family chips, all with prebuilt binaries: the classic
**ESP32**, **ESP32-S2**, **ESP32-S3**, **ESP32-C3**, and **ESP32-C6** (BLE WiFi
provisioning is only available on ESP32/S3/C3 — see the table below). Any of
the standard "DevKit" style boards for these chips works, and they all need
**4 MB of flash**. The classic **ESP32** is the simplest, cheapest, and most
common choice if you don't already have a specific board — that's the
configuration referenced everywhere else in these docs unless a page says
otherwise:

| Partition | Flash offset |
| --- | --- |
| Bootloader | `0x1000` (classic ESP32 / ESP32-S2), `0x0` (S3 / C3 / C6) |
| Partition table | `0x8000` |
| Firmware (single app, no OTA) | `0x10000` |
| LittleFS (web portal assets) | `0x370000` |

The default build uses a single-app layout without an OTA slot to fit the
firmware, the web portal, and your device configuration into 4 MB. Boards with
more flash work too and leave headroom for the optional
[Web OTA build](/gekko/guides/ota-updates/) (currently classic-ESP32 only).

You will also need a **USB data cable** and, on some boards, the usual
CP210x/CH340 USB-serial driver for your operating system.

## Peripherals from the device catalog

Everything below is optional — you add each one from the web portal when you
actually wire it up:

- **Relays / MOSFET boards** on any free GPIO (`gpio_switch`)
- **PCF8574 / PCF8575** I2C port expanders for more switch outputs
- **DS18B20** waterproof temperature probes on a 1-Wire bus (one GPIO, many
  probes)
- **NTC thermistors** and other analog sensors on an ADC pin, an **ADS1115**
  16-bit I2C ADC, or a **CD74HC4067** 16-channel multiplexer
- **HTU21** and **AHT10** I2C temperature + humidity sensors, or **DHT11** on
  one GPIO
- **SSD1306** OLED, **ST7735** TFT, **LCD1602/LCD2004** character displays, and
  four-digit **TM1637** modules
- **DS3231** I2C or three-wire **DS1302** real-time clocks — recommended if you
  use schedules and the device may run without internet/NTP
- **Peristaltic dosing pumps** driven through a switch output
- **LED drivers / PWM loads** on LEDC-capable pins (`analog_output`)
- Digital inputs: float switches, door contacts, leak sensors
  (`binary_sensor`)

See the [device catalog](/gekko/reference/devices/) for the full list of the 33
built-in device types.

:::tip
Start with just the bare board. Flash it, connect it to WiFi, and click around
the portal — you can add real hardware one device at a time later. Once
connected, set your exact board model on the **Controller board** settings
page *before* adding any pin-owning device — GPIO availability (ADC,
strapping, input-only, reserved pins) differs per board, and every device
form's pin picker is driven by this selection.
:::
