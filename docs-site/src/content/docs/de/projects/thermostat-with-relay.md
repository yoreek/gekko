---
title: Thermostat mit Relais
description: Bauen Sie eine sichere Temperaturregelung aus DS18B20, Relaisausgang und Thermostat.
sidebar:
  order: 1
---

Dieses Projekt steuert einen Heizer oder Kühler über einen DS18B20. Es verbindet
Verdrahtung, Abhängigkeiten, Automatisierung, Live-Status und Sicherheitsverhalten.

## Ergebnis

```text
DS18B20-Sensor → Thermostat → Relais → Heizer oder Kühler
```

## Hardware

- ESP32-Platine.
- DS18B20 mit 4,7-kΩ-Pull-up zwischen DATA und 3V3.
- Für die Last geeignetes Relaismodul.
- Heizer oder Kühler, über das Relais und nach dessen Sicherheitsvorgaben angeschlossen.

> Netzlasten niemals direkt an den ESP32 anschließen. Verwenden Sie ein passend
> ausgelegtes, geschlossenes Relais oder Schütz und beachten Sie die Elektrosicherheit.

## Gerätegraph und Reihenfolge

![Thermostat-Projektgraph. Zuerst 1-Wire-Bus, DS18B20 und GPIO-Schalter anlegen, danach den Thermostat.](../../../../assets/diagrams/de/thermostat-project-flow.svg)

1. [`onewire_bus`](/gekko/de/reference/devices/onewire-bus/) am Sensor-GPIO anlegen.
2. **Scan** ausführen und den gefundenen
   [`ds18b20_temperature_sensor`](/gekko/de/reference/devices/ds18b20/) anlegen.
3. Einen [`gpio_switch`](/gekko/de/reference/devices/gpio-switch/) für das Relais
   anlegen und den spannungslosen Zustand als Safe State setzen.
4. Relais mit dem GPIO-Schalter manuell testen.
5. [`thermostat`](/gekko/de/reference/devices/thermostat/) anlegen und Sensor
   sowie Schalter als Abhängigkeiten wählen.

Thermostat erst anlegen, wenn Sensor und Schalter `ready` sind.

## Minimale Einstellungen

Für einen Heizer Zieltemperatur und Hysterese setzen. Bei 25,0 °C mit 0,5 °C
Hysterese schaltet der Heizer unter 24,5 °C ein und bei 25,0 °C aus. Heiz- und
Kühlmodus haben entgegengesetzte Schaltrichtung. Vor dem Betrieb sichere Grenzen
setzen: Bei Sensorfehler muss der Ausgang in seinen Safe State zurückkehren.

## Prüfung und typische Probleme

1. Bus, Sensor, Schalter und Thermostat müssen `ready` sein.
2. Temperatur mit einem zuverlässigen Thermometer vergleichen.
3. Sollwert kurz ändern und das Schalten an der Hysteresegrenze prüfen.
4. Sensor in einer sicheren Testumgebung trennen: Ausgang muss Safe State erreichen.

- **Kein Sensor beim Scan:** DATA, 3V3/GND und 4,7-kΩ-Pull-up prüfen.
- **Relaislogik vertauscht:** Invertierung nur für aktiv-niedrige Relais aktivieren.
- **Abhängigkeit nicht wählbar:** kompatibles Gerät anlegen, aktivieren und auf `ready` warten.
