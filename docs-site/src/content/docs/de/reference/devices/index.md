---
title: Geraetekatalog
description: Alle 38 Geraetetypen, die in jedem Gekko-Firmware-Image eingebaut sind.
sidebar:
  order: 1
  label: Geraetekatalog
---

Jedes Gekko-Firmware-Image bringt alle diese Geraetetypen bereits mit - du
erzeugst Instanzen davon zur Laufzeit im Portal. Typen mit Link haben eine
eigene Referenzseite; die restlichen sind hier kurz beschrieben, mit
detaillierten Seiten, die nach und nach folgen.

## Busse & Infrastruktur

| Typ | Zweck |
| --- | --- |
| [`onewire_bus`](/gekko/de/reference/devices/onewire-bus/) | 1-Wire-Bus auf einem GPIO; Elternteil fuer DS18B20-Sonden, mit Geratescan |
| [`i2c_bus`](/gekko/de/reference/devices/i2c-bus/) | I2C-Bus (SDA/SCL); Elternteil fuer Displays, HTU21, RTC, Portexpander |
| [`spi_bus`](/gekko/de/reference/devices/spi-bus/) | SPI-Bus; Elternteil fuer das ST7735-TFT |
| `rtc_ds3231` | DS3231-Echtzeituhr - haelt Plaene ohne NTP am Laufen |
| `rtc_ds1302` | DS1302-Drei-Draht-Echtzeituhr an den GPIO-Pins CLK, DAT und RST |
| `dummy` | Platzhalter-/Testgeraet |

## Schalter & Ausgaenge

| Typ | Zweck |
| --- | --- |
| [`gpio_switch`](/gekko/de/reference/devices/gpio-switch/) | Ein/Aus-Ausgang auf einem GPIO - Relais, MOSFETs, LEDs |
| [`pcf8574_expander`](/gekko/de/reference/devices/port-expanders/) / [`pcf8575_expander`](/gekko/de/reference/devices/port-expanders/) | 8-/16-Bit-I2C-Portexpander |
| [`port_expander_switch`](/gekko/de/reference/devices/port-expanders/) | Ein Schalter auf einem Expander-Pin - dieselben Optionen wie ein GPIO-Schalter |
| [`analog_output`](/gekko/de/reference/devices/analog-outputs/) | LEDC-PWM-Ausgangskanal (dimmbares Licht, Luefter, ...) |
| [`fade_analog_output`](/gekko/de/reference/devices/analog-outputs/) | Sanfte Uebergaenge fuer einen Analogausgang |
| [`scheduled_analog_output`](/gekko/de/reference/devices/analog-outputs/) | Tagesprofil, das einen Analogausgang steuert |
| [`analog_output_composer`](/gekko/de/reference/devices/analog-outputs/) | Gruppiert Analogkanäle zu einer Leuchte - siehe das [Aquarienlicht-Beispiel](/gekko/de/reference/devices/analog-outputs/#beispiel-ein-fuenfkanaliges-aquariumlicht) |

## Lichteffekte

| Typ | Zweck |
| --- | --- |
| `pixel_strip` | WS2812B adressierbarer RGB-Streifen an einem GPIO — Adafruit-NeoPixel-Hardware-Backend |
| `pixel_effect_solid` | Füllt einen Ziel-`pixel_strip` mit einer einzigen Farbe |
| `pixel_effect_alert` | Lässt einen Ziel-`pixel_strip` blinken, solange UND-verknüpfte Bedingungen erfüllt sind — z. B. Überlauf-/Alarmanzeige |

## Analoge Eingaenge

| Typ | Zweck |
| --- | --- |
| [`analog_port_input`](/gekko/de/reference/devices/analog-inputs/) | Eine Spannungsmessung direkt von einem ESP32-ADC-Pin, ohne Zusatzhardware |
| [`ads1115_hub`](/gekko/de/reference/devices/analog-inputs/) | ADS1115 16-Bit-I2C-ADC - 4 praezise Kanaele |
| [`cd74hc4067_hub`](/gekko/de/reference/devices/analog-inputs/) | CD74HC4067 16-Kanal-Analog-Multiplexer an einem ADC-Pin |
| [`analog_input_channel`](/gekko/de/reference/devices/analog-inputs/) | Ein Kanal eines ADS1115- oder CD74HC4067-Hubs |

## Sensoren

| Typ | Zweck |
| --- | --- |
| [`ds18b20_temperature_sensor`](/gekko/de/reference/devices/ds18b20/) | DS18B20-1-Wire-Temperatursonde |
| [`ntc_thermistor_temperature_sensor`](/gekko/de/reference/devices/ntc-thermistor/) | NTC-Thermistor an einem beliebigen Analogeingang |
| [`htu21`](/gekko/de/reference/devices/htu21/) | HTU21 I2C-Temperatur- und Luftfeuchtesensor |
| `aht10` | AHT10 I2C-Temperatur- und Luftfeuchtesensor |
| `dht11` | DHT11 Temperatur- und Luftfeuchtesensor an einem GPIO |
| `binary_sensor` | Digitaler Eingang - Schwimmerschalter, Tuerkontakt, Lecksensor |

## Steuerung & Automatisierung

| Typ | Zweck |
| --- | --- |
| [`thermostat`](/gekko/de/reference/devices/thermostat/) | Hysterese-Heiz-/Kuehlregelung: Sensor rein, Schalter raus |
| [`schedule`](/gekko/de/reference/devices/schedule/) | Regeln fuer Zeitfenster und Wochentage mit Minutenpraezision |
| `auto_switch` | Steuert einen Schalter aus ver-AND-ten Bedingungen, mit Override und Pause - siehe [Schedules & Automation](/gekko/de/guides/schedules-and-automation/) |
| [`dosing_pump`](/gekko/de/reference/devices/dosing-pump/) | Dosierlaeufe mit Kalibrierung, Planung und Dosiereintrag |

## Displays

| Typ | Zweck |
| --- | --- |
| `ssd1306` | I2C-OLED mit dem [visuellen Layout-Designer](/gekko/de/guides/displays/) |
| `st7735` | SPI-Farb-TFT, derselbe Layout-Designer |
| `lcd1602` | HD44780-Zeichen-LCD mit 16 × 2 Zellen ueber ein eingebettetes PCF8574-I2C-Modul |
| `lcd2004` | HD44780-Zeichen-LCD mit 20 × 4 Zellen ueber ein eingebettetes PCF8574-I2C-Modul |
| `lcd1602_pin` | HD44780-Zeichen-LCD mit 16 × 2 Zellen, direkt an ESP32-GPIO-Pins angeschlossen |
| `lcd2004_pin` | HD44780-Zeichen-LCD mit 20 × 4 Zellen, direkt an ESP32-GPIO-Pins angeschlossen |
| `tm1637` | Vierstellige Siebensegmentanzeige mit Helligkeit und 180°-Drehung |
