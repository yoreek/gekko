---
title: Displays & Layout-Designer
description: Steuere SSD1306-OLED- und ST7735-TFT-Displays mit Gekkos visuellem Layout-Designer und Live-Metric-Platzhaltern.
sidebar:
  order: 3
---

Gekko steuert **SSD1306**-I2C-OLED-Displays und **ST7735**-SPI-TFT-Displays,
und das Portal bringt einen **visuellen Layout-Designer** mit - du setzt
zusammen, was auf dem Bildschirm erscheinen soll, direkt im Browser, mit
Vorschau.

## Ein Display einrichten

1. Erstelle zuerst das Bus-Geraet: ein
   [**I2C-Bus**](/gekko/de/reference/devices/i2c-bus/) (SDA/SCL-Pins) fuer
   SSD1306 oder ein [**SPI-Bus**](/gekko/de/reference/devices/spi-bus/) fuer
   ST7735.
2. Erstelle das Display-Geraet und waehle diesen Bus als Abhaengigkeit (plus
   I2C-Adresse oder TFT-Steuerpins).
3. Oeffne das Geraet und klicke auf **Design**, um in den Layout-Designer zu
   wechseln.

![Display-Layout-Designer](../../../../assets/screenshots/portal-display-designer.png)

## Seiten und Widgets

Ein Layout besteht aus **Seiten**; jede Seite enthaelt positionierte
**Widgets** (Text und mehr). Der Designer zeigt eine Live-Vorschau, die mit
denselben Fonts und denselben Metriken gerendert wird, die auch die Firmware
verwendet - was du siehst, ist das, was das Panel zeichnet. Layouts werden auf
dem Geraet gespeichert und sind in
[Backup-Bundles](/gekko/de/guides/backup-restore/) enthalten.

## Livewerte: Platzhalter fuer Metriken

Text-Widgets koennen statischen Text mit **Platzhaltern** mischen, die zur
Renderzeit aufgeloest werden. Eine Statusanzeige bauen ist nur ein paar Zeilen
Template-Text:

![Widget-Text mit Platzhaltern links, gerendertes OLED-Output mit Livewerten rechts](../../../../assets/diagrams/display-placeholders.svg)

Du musst dir die Syntax nicht merken - der **Platzhalter-Builder** im Designer
listet jede verfuegbare Metrik jedes Geraets mit einem Live-Vorschauwert und
setzt den Platzhalter fuer dich ein. Typisierte Platzhalter werden bei jedem
Tastendruck validiert. Weitere Beispiele:

```text
Room {{dev.670845748.temperature | fixed:1}}
IP {{system.wifi.station_ip}}
Now {{system.time | format:HH:mm}}
Up {{system.uptime}}
```

Platzhalter-Formen:

- `{{dev.<deviceId>.<metricKey>}}` - eine Metrik von einem beliebigen Geraet
  (Temperatur, Zustand, ...). Der Designer hat einen Platzhalter-Builder mit
  allen verfuegbaren Werten.
- `{{system.<metricKey>}}` - Systemmetriken wie `time` (Wanduhr) und
  `uptime` (Zeit seit dem Boot).
- `{{system.wifi.<metricKey>}}` - WiFi-Metriken wie `station_ip`.

Optionale Filter folgen hinter `|`:

| Filter | Beispiel | Wirkung |
| --- | --- | --- |
| `fixed:N` | `{{dev.123.temperature \| fixed:1}}` | Dezimalformat mit N Stellen |
| `format:pattern` | `{{system.time \| format:EEEE HH:mm}}` | Datums-/Zeitmuster (`YYYY MM DD HH mm ss EEEE`; `[literal]`-Text in Klammern) |
| `upper` / `lower` / `trim` | `{{system.wifi.station_ip \| upper}}` | Texttransformationen |

Ein Platzhalter, der nicht aufgeloest werden kann, wird als leerer Text
gerendert statt das ganze Widget zu zerstoeren - ein kurz fehlender Sensor
macht den Bildschirm also nicht leer.

Geraete, auf die Platzhalter verweisen, werden zu echten Registry-
Abhaengigkeiten des Displays - das Register warnt dich, bevor du einen Sensor
loeschst, den ein Display noch anzeigt.
