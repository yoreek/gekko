---
title: FAQ & troubleshooting
description: Common problems flashing, provisioning, and running a Gekko controller — and their fixes.
sidebar:
  order: 3
---

## Flashing

### The web installer says my browser is not supported

Web Serial only exists in Chromium browsers — use **Chrome, Edge, or Opera on
desktop**. Firefox, Safari, and all mobile browsers cannot flash. Alternatively
use the [esptool scripts](/gekko/getting-started/flashing/), which work
everywhere.

### The installer doesn't list my board's serial port

- Use a USB **data** cable — many bundled cables are charge-only.
- Install the CP210x or CH340 driver your board's USB-serial chip needs.
- On Linux, add yourself to the serial group (`sudo usermod -a -G dialout
  $USER`, then re-login) and note that some Linux + Chrome + USB-chip
  combinations are known-flaky over Web Serial — the esptool path is the
  reliable fallback.
- Close anything else holding the port (serial monitors, IDEs).

## First boot & WiFi

### The `gekko-…` setup access point never appears

- Give the board ~10 seconds after power-on.
- If the device was flashed before and holds old credentials, it goes straight
  to station mode — check your router's client list for its IP instead.
- Reflash with the erase option (the web installer offers "erase device"; with
  esptool, `esptool erase_flash` first) to return to a clean first boot.

### I'm connected to the setup AP but no portal opens

Not every OS pops the captive portal automatically. Open
`http://192.168.4.1/` in a browser yourself.

### I saved wrong WiFi credentials

Nothing is lost: connection retries are timeout-driven and the setup AP stays
available alongside them. Reconnect to the `gekko-…` AP and correct the
settings on the WiFi page.

## Portal & devices

### The portal loads but a device shows `dependency_blocked`

One of its dependencies is disabled, deleted, or faulted — e.g. a DS18B20
whose 1-Wire bus device is disabled. Fix the parent device first; the child
recovers on its own.

### My DS18B20 doesn't show up in the bus scan

Check the ~4.7 kΩ pull-up between data and 3.3 V, and the wiring. A healthy
probe scans with family code `28` and a 16-character address, without a CRC
flag.

### Schedules never turn anything on

Schedules require a plausible clock. Set the timezone and NTP on the **Time**
page, or add a DS3231 RTC. Also remember an auto switch must be in **Auto**
mode — a manual Off/On override ignores conditions, and an auto switch with no
conditions attached stays off by design.

### Where did the OTA / MQTT page go?

Those pages only appear on firmware builds compiled with the respective
feature — see [OTA updates](/gekko/guides/ota-updates/) and
[MQTT & Home Assistant](/gekko/guides/mqtt-home-assistant/).

## Recovery

### Factory reset

Reflash with a full erase (web installer's erase option, or
`esptool erase_flash` + reflash). This clears WiFi credentials and the whole
device registry — export a [backup](/gekko/guides/backup-restore/) first if you
want to restore the setup afterwards.

### Which firmware version am I running?

`GET /api/system/version`, the System page in the portal, or the
`Gekko booting version=…` line in the serial boot log.
