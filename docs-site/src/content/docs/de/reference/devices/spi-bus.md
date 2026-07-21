---
title: SPI-Bus
description: Wie der SPI-Bus funktioniert - geteilte Clock- und Datenleitungen, Chip-Select pro Geraet und das Ansteuern des ST7735-TFT in Gekko.
sidebar:
  order: 5
  label: SPI-Bus
---

## Was ist SPI?

SPI (Serial Peripheral Interface) ist der schnelle serielle Bus - waehrend
[I2C](/gekko/de/reference/devices/i2c-bus/) bei 400 kHz an sein Limit kommt,
taktspricht SPI muhelos mit Dutzenden MHz, weshalb Farbbildschirme ihn nutzen.
Der Preis sind mehr Leitungen: eine geteilte **Clock** (SCK) und **Data**
(MOSI, optional MISO fuer Rueckdaten), plus eine individuelle **Chip-Select**-
Leitung (CS) pro Geraet - so werden Geraete unterschieden, statt ueber
Adressen.

In Gekko ist der Bus das `spi_bus`-Geraet: Es besitzt die geteilten SCK-/
MOSI-/MISO-Pins und den ESP32-SPI-Host. Heute ist sein einziger Verbraucher
das **ST7735-Farb-TFT**-Display; die eigenen CS-/DC-/Reset-Pins des Displays
gehoeren zum Display-Geraet, nicht zum Bus.

## Verdrahtung

![SPI-Verdrahtung: geteiltes SCK und MOSI zum ST7735, Chip-Select und Data/Command-Pins pro Display](../../../../../assets/diagrams/spi-wiring.svg)

Ein typisches ST7735-Modul ist so verdrahtet (Aufdrucke auf dem Modul koennen
abweichen):

| Modul-Pin | ESP32-Pin (Standard) | Gehört zu |
| --- | --- | --- |
| SCK / CLK | GPIO 18 | `spi_bus` |
| SDA / MOSI / DIN | GPIO 23 | `spi_bus` |
| CS | GPIO 5 | `st7735`-Geraet |
| DC / A0 | GPIO 2 | `st7735`-Geraet |
| RES / RST | - (oder jeder freie GPIO) | `st7735`-Geraet |
| VCC, GND, LED/BLK | 3,3 V / GND | Versorgung |

Displays senden nie Daten zurueck, daher bleibt **MISO bei -1** (nicht
benutzt). Ein zweites SPI-Geraet wuerde SCK/MOSI teilen und einfach seinen
eigenen CS-Pin bekommen.

## Bus-Geraet und Diagnose

Wie die anderen Busse zeigt `spi_bus` Live-Diagnosen - Fehlerzaehler und den
Transaktionszustand - und wenn du es deaktivierst, wird das Display mit
`dependency_blocked` blockiert, statt alte Pixel stehenzulassen.

![SPI-Bus-Einstellungen mit Diagnose](../../../../../assets/screenshots/device-spi-bus.png)

## Konfiguration

| Feld | Standard | Bedeutung |
| --- | --- | --- |
| `host` | `VSPI` | Welcher ESP32-Hardware-SPI-Controller verwendet wird |
| `sckPin` | `18` | Geteilte Clock |
| `mosiPin` | `23` | Geteiltes Data-Out |
| `misoPin` | `-1` | Data-In - fuer Displays ungenutzt, nur setzen, wenn kuenftig ein Geraet Ruecklesen braucht |
| `enabled` | an | Das Deaktivieren des Busses blockiert jedes Geraet darauf |

## Naechster Schritt

Erstelle den Bus und fuege dann ein [ST7735-Display](/gekko/de/guides/displays/)
hinzu und oeffne den visuellen Layout-Designer - Seiten, Widgets und Live-
Metric-Platzhalter funktionieren genauso wie beim OLED.
