---
title: HTU21 Temperatur- & Luftfeuchtesensor
description: Der HTU21-I2C-Temperatur- und Luftfeuchtesensor in Gekko - ein kleines Board, das sowohl Lufttemperatur als auch relative Luftfeuchte meldet.
sidebar:
  order: 6.5
---

## Was ist ein HTU21?

Der HTU21 (und seine fast identischen Geschwister SHT21 / Si7021) ist ein
kleiner I2C-Sensor, der **zwei** Dinge gleichzeitig meldet: **Lufttemperatur**
(±0,3 °C) und **relative Luftfeuchte** (±2 % RH). Er kommt als kleines
Breakout in Fingernagelgroesse und ist damit die erste Wahl fuer die *Luft*
um einen Aufbau herum statt fuer das Wasser darin: Raumklima ueber einem
Aquarium, Luftfeuchte in einem Terrarium oder Vivarium, die Luft in einem
Grow-Tent oder Inkubator.

Waerend ein [DS18B20](/gekko/de/reference/devices/ds18b20/) eine wasserdichte
Sonde am Kabel fuer *Wassertemperatur* ist, ist der HTU21 ein auf dem Board
montierter Sensor fuer *Luft* - und er liefert ausserdem Luftfeuchte, die der
DS18B20 gar nicht messen kann.

## Verdrahtung: Es ist ein I2C-Geraet

Der HTU21 haengt wie jedes andere I2C-Peripheriegeraet am
[I2C-Bus](/gekko/de/reference/devices/i2c-bus/) - SDA, SCL, 3.3 V, GND, mit
Pull-ups, die auf dem Breakout fast immer schon vorhanden sind. Seine Adresse
ist fest bei **`0x40`** (keine Jumper), also kannst du pro Bus nur **einen**
HTU21 haben; fuer einen zweiten Luftsensor brauchst du einen zweiten I2C-Bus
auf anderen Pins.

## Einrichtung

1. Erstelle einen **[I2C-Bus](/gekko/de/reference/devices/i2c-bus/)** auf deinen
   SDA/SCL-Pins (falls du noch keinen hast) und nutze **Scan bus**, um zu
   bestaetigen, dass der Sensor bei `0x40` antwortet.
2. Erstelle ein **`htu21`**-Geraet und waehle diesen Bus als Abhaengigkeit.

![HTU21-Einstellungen: I2C-Bus-Auswahl, Adresse, Einheit und Reporting-Deltas](../../../../../assets/screenshots/device-htu21.png)

Damit ist es getan - zum Start brauchst du nichts zu kalibrieren. Das Geraet
meldet sofort Temperatur und Luftfeuchte, jeweils mit eigenem Gueltigkeits-
Flag: Ein abgezogener Sensor oder ein defekter Bus erscheint als *invalid*,
nie als alter Wert.

## Zwei Messwerte aus einem Geraet

Anders als die meisten Sensoren erzeugt ein HTU21 zwei Livewerte:

- **Temperatur** - liefert die `temperature_sensor`-Rolle, also kann sie ein
  [Thermostat](/gekko/de/reference/devices/thermostat/) steuern (z. B. eine
  Terrarium-Heizmatte), [Display-Platzhalter](/gekko/de/guides/displays/)
  speisen und in Home Assistant erscheinen.
- **Luftfeuchte** - als Prozentwert fuer Dashboard, Displays und Home Assistant.

Auf [MQTT-Builds](/gekko/de/guides/mqtt-home-assistant/) tauchen beide in Home
Assistant auf - ein `temperature`-Sensor und ein `humidity`-Sensor - und zwar
aus demselben Geraet.

## Kalibrierung

Da Temperatur und Luftfeuchte unabhaengige Messungen sind, hat jede ihren
eigenen Konditionierungsweg - einen Kalibrier-Offset/Faktor zum Abgleich mit
einer Referenz und ein Glättungsgewicht gegen Jitter. Das Trimmen der
Luftfeuchte (gegen ein kalibriertes Hygrometer oder eine Salztest-Referenz)
beruehrt die Temperatur nicht, und umgekehrt.

## Konfiguration

| Feld | Standard | Bedeutung |
| --- | --- | --- |
| `i2cAddress` | `0x40` | Feste HTU21-Adresse - normalerweise so lassen |
| `unit` | `celsius` | Anzeigeeinheit fuer Temperatur |
| `pollMs` | `5000` | Wie oft der Sensor gelesen wird |
| `reportDeltaCelsius` | `0.1` | Minimale Temperaturaenderung, bevor ein neuer Wert gesendet wird |
| `reportDeltaHumidity` | `0.1` | Minimale Luftfeuchteaenderung, bevor ein neuer Wert gesendet wird |
| `reportAlways` | aus | Jeden Poll senden, unabhaengig von den Deltas |
| `enabled` | an | Deaktivierte Geraete hoeren auf zu melden |

Lufttemperatur und Feuchte driften langsam - das Standard-Polling alle 5 s
mit kleinen Deltas haelt WebSocket und Historie ruhig, ohne etwas Echtes zu
verpassen.

## Bereitgestellt

- **temperature_sensor** - seine Temperatur kann ein Thermostat steuern oder
  einen Auto-Switch sperren.
