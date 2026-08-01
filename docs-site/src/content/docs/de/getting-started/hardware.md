---
title: Hardware-Anforderungen
description: Was du fuer Gekko brauchst - ein einfaches ESP32-Dev-Board mit 4 MB Flash.
sidebar:
  order: 2
---

## Das Controller-Board

Gekko laeuft auf fuenf ESP32-Chips, fuer alle gibt es vorgebaute Binaerdateien:
den klassischen **ESP32**, **ESP32-S2**, **ESP32-S3**, **ESP32-C3** und
**ESP32-C6** (BLE-WiFi-Provisioning gibt es nur fuer ESP32/S3/C3 - siehe
Tabelle unten). Jedes uebliche "DevKit"-Board fuer diese Chips funktioniert,
und alle brauchen **4 MB Flash**. Der klassische ESP32 ist die einfachste,
guenstigste und gebraeuchlichste Wahl, wenn du noch kein bestimmtes Board
hast - das ist die Konfiguration, auf die sich der Rest dieser Doku bezieht,
sofern nicht anders angegeben:

| Partition | Flash-Offset |
| --- | --- |
| Bootloader | `0x1000` (klassischer ESP32 / ESP32-S2), `0x0` (S3 / C3 / C6) |
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
- **HTU21**- und **AHT10**-I2C-Temperatur-/Feuchtesensoren oder **DHT11** an
  einem GPIO
- **SSD1306**-OLED, **ST7735**-TFT, **LCD1602/LCD2004**-Zeichenanzeigen und
  vierstellige **TM1637**-Module
- **DS3231**-I2C- oder Drei-Draht-**DS1302**-Echtzeituhren - empfohlen fuer
  Zeitplaene ohne Internet/NTP
- **Peristaltische Dosierpumpen** ueber einen Schaltausgang
- **LED-Treiber / PWM-Lasten** auf LEDC-faehigen Pins (`analog_output`)
- Digitale Eingaenge: Schwimmerschalter, Tuerkontakte, Lecksensoren
  (`binary_sensor`)

Siehe den [Geraetekatalog](/gekko/de/reference/devices/) fuer die komplette Liste
der 33 eingebauten Geraetetypen.

:::tip
Starte nur mit dem nackten Board. Flashen, mit WiFi verbinden und im Portal
umsehen - echtes Geraet kannst du spaeter eines nach dem anderen hinzufuegen.
:::
