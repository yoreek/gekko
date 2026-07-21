---
title: Hardware-Anforderungen
description: Was du fuer Gekko brauchst - ein einfaches ESP32-Dev-Board mit 4 MB Flash.
sidebar:
  order: 2
---

## Das Controller-Board

Gekko zielt auf den klassischen **ESP32** (den urspruenglichen Dual-Core-Chip)
mit **4 MB Flash** - also das uebliche "ESP32 DevKit"-Entwicklungsboard, das
oft nur wenige Euro kostet. Fuer diese Konfiguration sind die vorgebauten
Binärdateien gedacht:

| Partition | Flash-Offset |
| --- | --- |
| Bootloader | `0x1000` |
| Partitionstabelle | `0x8000` |
| Firmware (einzelne App, kein OTA) | `0x10000` |
| LittleFS (Webportal-Assets) | `0x370000` |

Das Standard-Build verwendet ein Single-App-Layout ohne OTA-Slot, damit
Firmware, Webportal und Geraetekonfiguration in 4 MB passen. Boards mit mehr
Flash funktionieren ebenfalls und lassen Luft fuer das optionale
[Web-OTA-Build](/gekko/de/guides/ota-updates/).

Du brauchst ausserdem ein **USB-Datenkabel** und, bei manchen Boards, den
passenden CP210x-/CH340-USB-Seriell-Treiber fuer dein Betriebssystem.

## Peripherie aus dem Geraetekatalog

Alles unten ist optional - du fuegst es im Webportal hinzu, wenn du es
tatsaechlich verdrahtest:

- **Relais / MOSFET-Boards** an jedem freien GPIO (`gpio_switch`)
- **PCF8574 / PCF8575** I2C-Portexpander fuer mehr Schaltausgaenge
- **DS18B20** wasserfeste Temperatursonden an einem 1-Wire-Bus (ein GPIO,
  viele Sonden)
- **NTC-Thermistoren** und andere analoge Sensoren an einem ADC-Pin, an einem
  **ADS1115** 16-Bit-I2C-ADC oder an einem **CD74HC4067** 16-Kanal-Multiplexer
- **HTU21** I2C-Temperatur- und Luftfeuchtesensor
- **SSD1306** I2C-OLED-Displays und **ST7735** SPI-TFT-Displays
- **DS3231** I2C-Echtzeituhr - empfohlen, wenn du Plaene nutzt und das Geraet
  auch ohne Internet/NTP laufen soll
- **Peristaltische Dosierpumpen** ueber einen Schaltausgang
- **LED-Treiber / PWM-Lasten** auf LEDC-faehigen Pins (`analog_output`)
- Digitale Eingaenge: Schwimmerschalter, Tuerkontakte, Lecksensoren
  (`binary_sensor`)

Siehe den [Geraetekatalog](/gekko/de/reference/devices/) fuer die komplette Liste
der 27 eingebauten Geraetetypen.

:::tip
Starte nur mit dem nackten Board. Flashen, mit WiFi verbinden und im Portal
umsehen - echtes Geraet kannst du spaeter eines nach dem anderen hinzufuegen.
:::
