---
title: OTA-Updates
description: Over-the-air Firmware-Updates in Gekko - was auf 4-MB-Boards funktioniert und was mehr Flash braucht.
sidebar:
  order: 6
---

Gekko hat zwei OTA-Pfade, beide **im Standard-Build ausgeschaltet** - dem
klassischen 4-MB-ESP32-Board fehlt schlicht der Flash-Spielraum fuer ein
OTA-Partitionsschema neben Firmware, Webportal und Konfiguration.

## Standard-Build: Update ueber Serial

Im normalen Single-App-Layout laufen Updates als [Neuflash ueber USB](/gekko/de/getting-started/flashing/).
Deine Geraetekonfiguration liegt in NVS und ueberlebt einen Firmware-Neuflash -
ein [Backup-Bundle](/gekko/de/guides/backup-restore/) solltest du vor einem
Update trotzdem haben.

## PlatformIO OTA Upload (fuer Entwickler)

Fuer Boards mit genug Flash und OTA-faehigem Partition-Layout liefert die
PlatformIO-Umgebung `esp32dev_ota` dasselbe Firmware-Image ueber das Netz
anstatt ueber Serial:

```sh
pio run -e esp32dev_ota -t upload
```

Es ist bewusst nur ein Upload-Transport-Alias von `esp32dev` - gleiches
Image, gleiche Build-Flags - damit Serial- und OTA-Auslieferung byte-identisch
bleiben.

## Web OTA (Portal-Upload)

Firmware mit der geschuetzten Web-OTA-Option bekommt im Portal eine **OTA**-
Seite: Du laedst ein Firmware-Image im Browser hoch, und das Geraet
verifiziert es, schliesst den Vorgang ab und startet neu. Zu grosse oder
unterbrochene Uploads lassen die laufende Firmware unangetastet. Auf Builds
ohne dieses Feature blendet das Portal die OTA-Seite einfach aus.

:::note
Behandle Web OTA als Entwicklungs-/Advanced-Feature: aktiviere es nur auf
Boards mit genug Flash-Spielraum, gemaess `docs/platformio-environments.md` im
Repository.
:::
