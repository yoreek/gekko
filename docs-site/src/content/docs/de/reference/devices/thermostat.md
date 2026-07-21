---
title: Thermostat
description: Wie Gekkos Thermostat eine Temperatur auf dem Sollwert haelt - die Regelung, Hysterese und die Sicherheitsgrenzen darum.
sidebar:
  order: 7
---

## Was es tut

Ein Thermostat schliesst den Regelkreis zwischen Temperatursensor und
Schalter: *Wenn das Wasser zu kalt ist, schalte den Heizer ein; sobald es
warm genug ist, schalte ihn aus.* In Gekko ist das ein `thermostat`-Geraet,
das an zwei andere Geraete verdrahtet wird:

![Regelkreis: DS18B20 misst, Thermostat entscheidet, Relais treibt den Heizer, Wasser erwärmt sich, wiederholen](../../../../../assets/diagrams/thermostat-loop.svg)

Das funktioniert auch zum Kuehlen - **cool**-Modus treibt einen Kuehler oder
einen Luefter mit derselben, nur gespiegelten Logik, und **off** parkt den
Ausgang.

## Hysterese: warum es nicht flattert

Ein naives "ein bei unter 25,0, aus bei ueber 25,0" wuerde das Relais Dutzende
Male pro Minute klappern lassen, waehrend der Wert um den Sollwert schwankt.
Die Loesung ist ein **Totband** - die Hysterese:

![Hysterese-Diagramm: Heizer an unter 24,5, aus bei 25,0, nichts schaltet innerhalb des Bandes](../../../../../assets/diagrams/thermostat-hysteresis.svg)

Mit Sollwert 25,0 °C und Hysterese 0,5 °C im Heat-Modus:

- der Heizer geht **an**, wenn die Temperatur auf **24,5** faellt (Sollwert -
  Hysterese);
- er bleibt an, bis die Temperatur **25,0** erreicht, dann geht er **aus**;
- dazwischen schaltet nichts - die Temperatur darf durch das Band driften.

Groessere Hysterese = weniger Relaiszyklen, aber groessere Temperaturschwankung;
kleinere = engere Regelung, aber mehr Schalten. Fuer einen Aquarium-Heizer sind
0,3-0,5 °C sinnvoll. Zusaetzlich erzwingt **min switch interval** (Standard 5 s)
eine harte Mindestzeit zwischen Ausgangswechseln - guenstige Versicherung fuer
Relais, essenziell fuer kompressorbasierte Kuehler, die nicht kurzgetaktet
werden duerfen.

## Sicherheitsgrenzen

Das Thermostat nimmt an, dass gelegentlich etwas schiefgeht, und faellt in
Richtung "Heizer aus":

- **Sicherer Bereich** (`minSafeCelsius` / `maxSafeCelsius`) - ein Wert ausserhalb
  dieses Fensters gilt als Fehler (Sensor aus dem Wasser gefallen, Leitung auf
  einen festen Wert gebrochen): Der Ausgang geht in seinen Safe State und der
  Status zeigt `out_of_range`.
- **Sensor-Timeout** - kein frischer Wert innerhalb von `sensorTimeoutMs`
  (toter Bus, deaktivierter Sensor) stoppt ebenfalls das Heizen:
  `sensor_timeout`.
- **Retry-Backoff** - nach einem Fehler wartet das Thermostat
  `retryAfterErrorMs`, bevor es erneut versucht, statt jede Sekunde auf einen
  kaputten Sensor zu hämmern.
- Der **eigene Safe State des Schalters** deckt den umgekehrten Fehler ab -
  wenn das Thermostat selbst deaktiviert oder geloescht wird, faellt der
  [Schalter zurueck](/gekko/de/reference/devices/gpio-switch/) auf den dort
  konfigurierten Zustand.

## Einrichtung

1. Erstelle den [DS18B20-Sensor](/gekko/de/reference/devices/ds18b20/) (oder NTC/HTU21).
2. Erstelle den [Schalter](/gekko/de/reference/devices/gpio-switch/), der das
   Relais des Heizers treibt. Heizungen sind ein Fall, bei dem du an
   `safeState: off` und `startupState: off` denken solltest.
3. Erstelle das **Thermostat**: waehle Sensor und Schalter, setze Modus,
   Sollwert und Hysterese.

![Thermostat-Einstellungen im Portal](../../../../../assets/screenshots/device-thermostat.png)

## Konfiguration

| Feld | Standard | Bedeutung |
| --- | --- | --- |
| `mode` | `heat` | `heat`, `cool` oder `off` |
| `targetCelsius` | `25` | Der Sollwert |
| `hysteresisCelsius` | `0.5` | Das Totband unterhalb (heat) oder oberhalb (cool) des Sollwerts |
| `minSafeCelsius` / `maxSafeCelsius` | `0` / `50` | Fehlergrenzen fuer den Sensorwert |
| `checkIntervalMs` | `1000` | Zeitraum des Regelkreises |
| `sensorTimeoutMs` | `6000` | Maximales Alter eines Werts vor `sensor_timeout` |
| `minSwitchIntervalMs` | `5000` | Mindestzeit zwischen Ausgangswechseln |
| `retryAfterErrorMs` | `30000` | Backoff vor erneuten Versuchen nach einem Fehler |

## Laufzeit & Home Assistant

Die Laufzeit meldet die aktuelle Temperatur, den Ausgangszustand und einen
Status - `heating`, `cooling`, `idle`, `sensor_timeout`, `out_of_range`,
`dependency_blocked` - sichtbar mit Icons im Portal und im Geraeteevent-Journal
gespeichert. Auf [MQTT-Builds](/gekko/de/guides/mqtt-home-assistant/) erscheint
das Thermostat in Home Assistant als volle `climate`-Entity (Modus, Sollwert,
aktuelle Temperatur, Aktion), und Sollwertaenderungen aus HA werden gegen den
sicheren Bereich validiert.
