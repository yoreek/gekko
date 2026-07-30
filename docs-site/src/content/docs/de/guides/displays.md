---
title: Displays & Layout-Designer
description: Konfiguriere Pixel-, Zeichen- und Siebensegmentanzeigen mit Gekkos visuellem Layout-Designer.
sidebar:
  order: 3
---

Gekko unterstuetzt fuenf Displaytypen ueber einen gemeinsamen **visuellen
Layout-Designer**. Seiten und Widgets werden mit Vorschau im Browser
konfiguriert; Koordinaten und Widgets passen sich dem Display an.

| Typ | Hardware | Koordinaten | Widgets |
| --- | --- | --- | --- |
| `ssd1306` | Monochromes I2C-OLED | Pixel | Text, Formen und Bitmaps |
| `st7735` | SPI-Farb-TFT | Pixel | Text, Formen und RGB565-Bitmaps |
| `lcd1602` | HD44780 16 × 2 ueber PCF857x | Zeichenzellen | Character |
| `lcd2004` | HD44780 20 × 4 ueber PCF857x | Zeichenzellen | Character |
| `tm1637` | Vierstellige Siebensegmentanzeige | Ziffernpositionen | Digital |

## Ein Display einrichten

1. Erstelle zuerst die erforderlichen Geraete:
   - einen [**I2C-Bus**](/gekko/de/reference/devices/i2c-bus/) fuer SSD1306;
   - einen [**SPI-Bus**](/gekko/de/reference/devices/spi-bus/) fuer ST7735;
   - einen [**Portexpander**](/gekko/de/reference/devices/port-expanders/)
     PCF8574/PCF8575 fuer LCD1602/LCD2004;
   - nichts fuer TM1637: es steuert seine CLK- und DIO-Pins direkt.
2. Erstelle das Display, waehle diese Geraete als Abhaengigkeiten und
   konfiguriere Adresse, Verdrahtung, Steuerpins, Helligkeit oder Drehung.
3. Oeffne das Geraet und klicke auf **Design**, um in den Layout-Designer zu
   wechseln.

![Display-Layout-Designer](../../../../assets/screenshots/portal-display-designer.png)

## Seiten und Widgets

Ein Layout besteht aus **Seiten** mit positionierten **Widgets**. Pixelanzeigen
verwenden Pixelkoordinaten, LCD1602/LCD2004 Zeichenzellen und TM1637
Ziffernpositionen. Der Designer erlaubt nur die vom Display unterstuetzten
Widgettypen und zeigt eine passende Vorschau. Layouts werden auf dem Geraet
gespeichert und sind in
[Backup-Bundles](/gekko/de/guides/backup-restore/) enthalten.

## Livewerte: Platzhalter fuer Metriken

Text-, Character- und Digital-Widgets koennen statischen Text mit
**Platzhaltern** mischen, die zur Renderzeit aufgeloest werden.

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

Ein Platzhalter, der nicht aufgeloest werden kann, wird als `N/A` gerendert
statt das ganze Widget zu zerstoeren - ein kurz fehlender Sensor macht den
Bildschirm also nicht leer, sondern zeigt an dieser Stelle `N/A` an.

Geraete, auf die Platzhalter verweisen, werden zu echten Registry-
Abhaengigkeiten des Displays - das Register warnt dich, bevor du einen Sensor
loeschst, den ein Display noch anzeigt.
