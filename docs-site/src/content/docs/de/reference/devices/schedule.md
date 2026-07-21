---
title: Schedule
description: Referenz fuer Gekkos Schedule-Geraetetyp - Zeitregeln fuer den Tag mit Minutenpraezision und Wochentagen.
sidebar:
  order: 8
---

`schedule` haelt eine Menge Zeitregeln und beantwortet nur eine Frage: *Ist
dieser Zeitplan gerade aktiv?* Er hat keinen eigenen Ausgang - haenge ihn als
Bedingung an einen [Auto-Switch](/gekko/de/guides/schedules-and-automation/)
(oder an die Planung einer Dosierpumpe), damit etwas passiert.

![Schedule-Regel-Editor](../../../../../assets/screenshots/device-schedule.png)

## Abhaengigkeiten

Keine. Andere Geraete haengen vom Schedule ab, nicht umgekehrt.

## Konfiguration

Bis zu **4 Regeln**, per OR verknuepft - der Schedule ist aktiv, wenn eine
beliebige aktivierte Regel passt. Jede Regel hat:

| Feld | Bedeutung |
| --- | --- |
| Wochentage | An welchen Tagen die Regel gilt |
| Start-/Endzeit | Das aktive Fenster, in Minuten des Tages (Minutenpraezision - keine Sekunden) |
| Modus | **Always on** - das ganze Fenster aktiv; **Interval** - das Fenster in N gleiche Abschnitte teilen, aktiv fuer die ersten M Minuten jedes Abschnitts |

Der Interval-Modus deckt periodische Aufgaben ab: z. B. ein 08:00-20:00-
Fenster mit 12 Intervallen und 5 Minuten Dauer laesst eine Umwaelzpumpe jede
Stunde 5 Minuten laufen.

## Zeit & Uhr

Die Regeln werden gegen die Uhr und die konfigurierte Zeitzone des Geraets
bewertet (DST wird automatisch behandelt). Bis die Uhr plausibel ist - NTP
synchronisiert oder eine DS3231-RTC vorhanden - meldet der Schedule sich als
nicht gueltig, und abhängige Geraete halten ihre Ausgaenge sicher.

Der Editor im Portal zeigt eine Ein/Aus-Vorschau und die naechste Umschaltung,
im Browser aus denselben Regeln berechnet; sie ist als Schaetzung markiert,
weil Browser-Uhr und Zeitzone von denen des Geraets abweichen koennen.

## Bereitgestellt

- **condition** - fuer Auto-Switches und verkettete Automatisierungen.
- **schedule** - fuer Geraete, die Plaene direkt konsumieren.
