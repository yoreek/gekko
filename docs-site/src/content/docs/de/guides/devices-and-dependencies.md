---
title: Geraete und Abhaengigkeiten
description: Wie Gekkos typisiertes Geraeteregister und der Abhaengigkeitsgraph funktionieren.
sidebar:
  order: 1
---

Die Kernidee hinter Gekko ist ein **Geraeteregister**: eine persistierte Liste
von Geraeteinstanzen, jede auf Basis eines der eingebauten
[Geraetetypen](/gekko/de/reference/devices/) erstellt, mit eigener Konfiguration
und eigenem Laufzeitstatus.

## Geraete werden zusammengesetzt, nicht isoliert konfiguriert

Reale Hardware ist geschichtet - ein Sensor sitzt auf einem Bus, ein Schalter
sitzt hinter einem Portexpander, eine Automatisierung steuert einen Schalter.
Gekko bildet das direkt ab: Ein Geraet **deklariert Abhaengigkeiten** auf
andere Geraete, ueber Rollen. Beispiele:

| Dieses Geraet ... | ...haengt ab von |
| --- | --- |
| DS18B20-Temperatursonde | einem 1-Wire-Bus-Geraet (das den GPIO besitzt) |
| SSD1306-OLED-Display | einem I2C-Bus-Geraet |
| Schalter auf einem PCF8574 | dem Portexpander-Geraet |
| Thermostat | einem Temperatursensor **und** einem Schalter |
| Auto-Switch | einem echten Schalter plus bis zu 6 Bedingungs-Geraeten |
| Zeitgesteuerter Analogausgang | einem Analogausgangskanal |

Das Register validiert den Graphen, wenn du ein Geraet erstellst oder
bearbeitest - du kannst kein Display an ein Geraet haengen, das kein I2C-Bus
ist, und du kannst keinen Bus loeschen, waehrend ein Sensor noch davon
abhaengt. Abhaengigkeiten werden in den Portal-Dialogen bereits vorgefiltert
aus kompatiblen Geraeten ausgewaehlt.

## Rollen statt fest verdrahteter Paare

Abhaengigkeiten werden ueber **Rollen** gematcht (`switch`,
`temperature_sensor`, `i2c_bus`, `condition`, ...), und ein Geraetetyp kann
mehrere Rollen bereitstellen. Ein GPIO-Schalter ist sowohl `switch` als auch
`condition`, daher kann ein Auto-Switch ihn entweder als Ausgang oder als
Eingangsbedingung verwenden. Ein Auto-Switch selbst stellt ebenfalls `switch`
und `condition` bereit, also koennen Automatisierungen verkettet werden.

## Konfiguration vs. Laufzeitstatus

Jedes Geraet trennt:

- **Config** - persistierte Einstellungen (Name, Pins, Regeln,
  Abhaengigkeiten). Sie werden in versionierter Binärform auf dem Geraet
  gespeichert und bei Firmware-Upgrades automatisch migriert. Das enthalten
  [Backup-Bundles](/gekko/de/guides/backup-restore/).
- **Runtime** - Live-Zustand (ein/aus, Temperatur, Status wie `ready` oder
  `dependency_blocked`). Er wird nie in der Config gespeichert, sondern in
  Echtzeit per WebSocket ins Portal gestreamt.

Einige Typen halten zusaetzlich einen kleinen **Retained State** ueber
Reboots hinweg - zum Beispiel den letzten Ausgangszustand eines Schalters
(wenn "Restore previous state" aktiv ist) oder den pausierten Countdown eines
Auto-Switches - ohne die Config neu zu schreiben.

## Lebenszyklus

Geraete koennen **aktiviert/deaktiviert** werden, ohne geloescht zu werden,
und jede Instanz meldet einen Status, den das Portal anzeigt: Ein Sensor mit
fehlendem Bus zeigt `dependency_blocked`, ein fehlerhaftes Geraet zeigt seinen
Fehler, und das **Device events**-Journal zeichnet die Uebergaenge auf.
