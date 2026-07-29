---
title: What is Gekko?
description: An introduction to Gekko — a modular ESP32 device controller with a built-in web portal.
sidebar:
  order: 1
---

Gekko is firmware for the ESP32 plus a web portal served straight from the
device's own flash. Together they let you build your own controller —
aquarium, terrarium, greenhouse, or general home automation — out of a catalog
of device types, wired together and configured entirely through the UI.

**One firmware image, no per-project rebuild.** Every supported device type is
already built in. Adding a relay, a temperature sensor, a display, or a dosing
pump is a web portal action against the running device, never a recompile.

## What you can build with it

- **Switches and outputs** — GPIO relays, switches behind PCF8574/PCF8575 I2C
  port expanders, PWM/analog outputs with smooth fade transitions, daily
  brightness curves, and multi-channel grouping.
- **Sensors** — DS18B20 and NTC temperature sensors, HTU21/AHT10/DHT11
  temperature + humidity sensors, and digital binary inputs.
- **Automation** — minute-precision daily schedules, condition-driven auto
  switches with manual override and pause, hysteresis thermostats, and dosing
  pumps with calibration and a dose journal.
- **Displays** — SSD1306 OLED, ST7735 TFT, LCD1602/LCD2004 character displays,
  and TM1637 seven-segment modules with a shared visual layout designer.
- **Infrastructure** — I2C/SPI/1-Wire buses, DS3231/DS1302 real-time clocks,
  and a dashboard you compose from panels.

Devices declare **dependencies** on each other — a switch on a port expander, a
sensor on an I2C bus, a pump gated by a schedule — and the registry validates,
enforces, and persists that graph. See
[Devices and dependencies](/gekko/guides/devices-and-dependencies/) for the
concept.

## What makes Gekko different

**No firmware image per setup.** Many controller firmwares turn your
configuration into a dedicated build — adding a sensor means editing a config
file, recompiling, and reflashing. Gekko ships one image with every supported
device type built in; changing your setup is always a portal action against
the running device, never a rebuild.

**Structure instead of pin templates.** Configuring at runtime usually means a
flat list of GPIO assignments and console rules. Gekko instead models your
hardware the way it is actually wired: a typed registry of devices with
declared dependencies, each with its own versioned config that migrates
automatically across firmware upgrades.

**Everything is observable and scriptable.** Live state streams over a
WebSocket, every device speaks the same REST API, notable events land in a
journal, displays get a visual designer, and the dashboard is composed from
panels — not a single console screen.

**One-toggle Home Assistant.** On MQTT-enabled builds, publishing a device to
Home Assistant is a single switch on its page — it appears there as a native
entity (switch, sensor, climate) you can control from HA, while everything
keeps running locally. See
[MQTT & Home Assistant](/gekko/guides/mqtt-home-assistant/).

The honest trade-off: the device-type catalog is fixed at compile time, so it
is deliberately a smaller, more structured base to build on rather than a
catalog of every sensor ever made.

## Everything runs on the device

- **Local-first** — the portal is served from the ESP32 over WiFi; no cloud, no
  account, no app store.
- **Optional integrations** — MQTT + Home Assistant discovery and OTA updates
  exist but are off by default.
- **WiFi provisioning** — a setup access point (or BLE, when enabled) gets the
  device onto your network without any hardcoded credentials.
- **Backup and restore** — the full device setup exports as a single
  human-editable bundle.
- **Seven languages** — the portal auto-detects your browser language and ships
  in English, Ukrainian, Russian, German, Spanish, French, and Italian.

## Next steps

1. [Check the hardware requirements](/gekko/getting-started/hardware/)
2. [Flash the firmware](/gekko/getting-started/flashing/) — from your browser,
   or with esptool/PlatformIO
3. [Connect the device to WiFi](/gekko/getting-started/first-boot-wifi/)
4. [Add your first device](/gekko/getting-started/first-device/)
