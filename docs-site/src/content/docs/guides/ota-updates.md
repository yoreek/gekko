---
title: OTA updates
description: Over-the-air firmware updates in Gekko — what works on 4 MB boards and what needs more flash.
sidebar:
  order: 6
---

Gekko has two over-the-air paths, both **off in the default build** — the
standard 4 MB ESP32 board simply doesn't have the flash headroom for an OTA
partition scheme next to the firmware, the web portal, and your configuration.

## Default build: update over serial

On the stock single-app layout, updates are a [reflash over
USB](/gekko/getting-started/flashing/). Your device setup is stored in NVS and
survives a firmware reflash — and you should still keep a
[backup bundle](/gekko/guides/backup-restore/) around before updating.

## PlatformIO OTA upload (developers)

For boards with enough flash and an OTA-enabled partition layout, the
`esp32dev_ota` PlatformIO environment delivers the same firmware image over the
network instead of serial:

```sh
pio run -e esp32dev_ota -t upload
```

It is deliberately an upload-transport alias of `esp32dev` — same image, same
build flags — so serial and OTA deliveries stay byte-identical.

## Web OTA (portal upload)

Firmware built with the guarded Web OTA option adds an **OTA** page to the
portal: upload a firmware image from the browser, and the device verifies,
finalizes, and reboots into it. Oversized or interrupted uploads leave the
running firmware untouched. On builds without the feature the portal simply
hides the OTA page.

:::note
Treat Web OTA as a development/advanced feature: enable it only on boards with
flash headroom, per `docs/platformio-environments.md` in the repository.
:::
