---
title: Dosierpumpe
description: Was eine Dosierpumpe ist, warum Aquarianer dosieren automatisieren, und wie Gekkos dosing_pump jeden Milliliter plant, kalibriert und protokolliert.
sidebar:
  order: 9
---

## Was ist eine Dosierpumpe?

Eine Dosierpumpe ist eine langsame, praezise Fluessigkeitspumpe. Der uebliche
Typ ist **peristaltisch**: Ein kleiner Motor drueckt mit Rollen auf einen
weichen Silikonschlauch und foerdert so ein paar Milliliter pro Sekunde. Da
die Fluessigkeit nur den Schlauch beruehrt, kann die Pumpe sie nicht
kontaminieren - und weil die Foerderrate konstant ist, entspricht *Laufzeit
direkt Millilitern*.

Aquarianer (und Gaertner) nutzen sie ueberall dort, wo eine Fluessigkeit
**klein und oft** zugegeben werden muss:

- **Riffbecken** - Calcium, Alkalinitaet (KH), Magnesium, Spurenelemente.
  Korallen verbrauchen das kontinuierlich; taegliche Mikro-Dosen halten die
  Wasserchemie viel stabiler als eine grosse woechentliche Korrektur.
- **Pflanzbecken** - taeglicher Fluessigduenger statt "wenn ich dran denke".
- **Teiche/Gewaechshaeuser** - pH-Puffer, Nährstoffe.

Manuelles Dosieren bedeutet Messbecher, Kalender und unvermeidliche Aussetzer.
Eine automatisierte Dosierpumpe macht jeden Tag zur selben Minute dasselbe -
genau diese Konstanz ist der Sinn.

## Die Teile und wie Gekko sie verbindet

![Dosierpumpen-Setup: Behaelter mit Schwimmsensor, peristaltische Pumpe ueber Relais, ESP32, Aquarium](../../../../../assets/diagrams/dosing-setup.svg)

Du brauchst vier guenstige Teile, und jedes davon entspricht einem Gekko-
Geraet:

| Hardware | Gekko-Geraet | Rolle |
| --- | --- | --- |
| Peristaltische Pumpe + Relais-/MOSFET-Board (orange Draht oben) | `gpio_switch` (oder `port_expander_switch`) | Der Ausgang, den das Pumpengeraet ein-/ausschaltet |
| Die Dosierpumpe selbst (Logik) | `dosing_pump` | Besitzt Plan, Kalibrierung, Behaelter und Historie |
| Flasche/Behaelter mit Loesung | - (im `dosing_pump`-Config mitverfolgt) | Das, woraus du dosierst |
| Optionaler Schwimmschalter in der Flasche (gruer Draht oben) | `binary_sensor` | Meldet Gekko, dass die Flasche leer ist, egal was der Zaehler sagt |

Mehrere Pumpen? Erstelle eine Kette pro Fluessigkeit - ein typisches Riff-
Gestell laeuft mit zwei oder drei (z. B. Calcium, Alkalinitaet, Magnesium)
nebeneinander, jede mit eigener Flasche und eigenem Plan.

## Einrichtung

1. Erstelle einen **GPIO Switch** auf dem Pin, der das Pumpenrelais treibt
   (siehe [dein erstes Geraet](/gekko/de/getting-started/first-device/) - der
   Ablauf ist derselbe).
2. Optional einen **Binary Sensor** fuer den Schwimmschalter.
3. Erstelle das **Dosierpumpen**-Geraet: Waehle den Schalter als *pump switch*,
   den Sensor als *low-level sensor* (pro Verbindung invertierbar), setze
   Behaeltervolumen und Warnschwelle und fuege Dosierslots zum Plan hinzu.

![Dosierpumpen-Einstellungen im Portal](../../../../../assets/screenshots/device-dosing-pump.png)

## Vor dem Vertrauen kalibrieren

Gekko wandelt Milliliter in Laufzeitsekunden ueber eine einzige Zahl um - die
Foerderrate der Pumpe (`ml/s`). Schlauchlaenge, Durchmesser und Foerderhoehe
aendern sie, also miss sie einmal mit deinem echten Aufbau:

![Kalibrierung: eine Dosis laufen lassen, echtes Volumen messen, eintragen](../../../../../assets/diagrams/dosing-calibration.svg)

Kalibrierlaeufe werden aus Statistiken und Historie ausgeschlossen, aber die
tatsaechlich abgegebene Fluessigkeit wird **sehr wohl** vom Behaelter
abgezogen - sie ist wirklich aus der Flasche raus. Wenn du die Foerderrate
bereits kennst, gibt es einen direkten Modus zum Eintragen.

## Zeitplanung: wie Dosen wirklich starten

Der Plan enthaelt Dosierslots (Uhrzeit + Menge) und ein Tagesmuster - alle N
Tage oder bestimmte Wochentage. Das Geraet bewertet ihn gegen seine eigene
Uhr, also [gib ihm eine verlaessliche Zeitquelle](/gekko/de/reference/devices/schedule/#zeit--uhr).
Die Tagesmenge wird absichtlich auf mehrere kleine Dosen verteilt - auch hier:
Stabilitaet.

![Dosis-Zeitlinie: pünktliche Dosis, Dosis innerhalb des 5-Minuten-Fensters, verpasste Dosis wird uebersprungen](../../../../../assets/diagrams/dosing-timeline.svg)

Zwei Regeln sind wichtig:

- **Grace window.** Ein Slot darf bis zu 5 Minuten spaeter starten - etwa wenn
  eine manuelle Dosis oder Kalibrierung den Pumpen-Minutenpunkt belegt hat.
- **Verspaetet nicht nachdosen.** Ein Slot, der laenger als das Grace Window
  verpasst wurde, wird *uebersprungen*, nie nachgeholt. Nach einem Reboot,
  einer Uhr, die mitten am Tag synchronisiert, oder einer langen Kalibrierung
  bekommst du **keine** Nachhol-Burst-Dosen - fuer Wasserchemie ist eine
  spaete Burst-Dosis schlimmer als eine verpasste Mikro-Dosis.

Pro Slot gibt es ausserdem **skip next** fuer genau einen naechsten Durchlauf
(Wasserwechsel, Urlaub) und den **auto**-Schalter, der den gesamten Plan
sperrt, waehrend manuelle Dosen weiter funktionieren.

Eine Dosis laeuft nach dem Start vollstaendig auf dem Geraet - das Portal
sendet nur den Befehl. Du kannst den Browser mitten in der Dosis schliessen;
die Firmware misst die Zeit, schaltet die Pumpe aus und bucht die abgegebene
Menge. Gleichzeitig kann nur ein Lauf aktiv sein (manuell, geplant oder
Kalibrierung - nichts preemptet etwas anderes), und alles, was das Geraet aus
dem Dienst nimmt, stoppt den Motor hart.

## Behaelter-Tracking

Gib Gekko die Kapazitaet der Flasche, und es zaehlt jeden Milliliter:

![Behaelter-Tracking: Zaehler sinkt, Low-Level-Warnung, Leer blockiert Auto-Dosierung](../../../../../assets/diagrams/dosing-container.svg)

- Unterhalb der **Warnschwelle** loest das Portal einen Alarm aus (Glocke +
  Toast) - Zeit fuer eine neue Mischung.
- **Leer** - Zaehler bei Null oder Schwimmsensor ausgeloest - loest einen
  kritischen Alarm aus, und wenn **block auto dosing when empty** aktiviert
  ist, stoppen geplante Dosen statt die Pumpe trocken laufen zu lassen.
- **`daysLeft`** schaetzt, wie lange das Restvolumen bei der durchschnittlichen
  taeglichen Aufnahme des Plans reicht.
- Nach dem Nachfuellen den Stand mit dem Befehl **Set volume** eintragen.

## Dosisjournal

Jede geplante und manuelle Dosis wird in ein on-device Journal geschrieben -
90+ Tage Historie bei typischen Dosiermengen, in einer eigenen Flash-Partition
gespeichert, sodass Firmware- und Portal-Updates sie ueberstehen. Die
Gerateseite zeichnet sie als Diagramm; `GET /api/dosejournal?deviceId=<id>&periodDays=<n>`
liefert sie roh. Das Journal ist ein fester Ring pro Pumpe: alte Eintraege
rotieren von selbst weg, und die Historie einer Pumpe kann nie die einer
anderen verdrängen.

## Befehle (REST)

| Befehl | Payload | Effekt |
| --- | --- | --- |
| `startDose` | `amountMl`, `logging` | Manuelle Dosis (`logging:false` = Kalibrierlauf) |
| `stopDose` | - | Sofort stoppen, die tatsaechliche Menge buchen |
| `setVolume` | `volumeMl` | Behaelter auffuellen/korrigieren |
| `skipNext` | `doseIndex`, `skip` | Eine kommende Ausfuehrung eines Slots ueberspringen |
| `setMode` | `auto` / `manual` | Scheduler aktivieren/deaktivieren |

Vollstaendiges Ausfuehrungsmodell und Journal-Interna:
[`docs/dosing-pump.md`](https://github.com/yoreek/gekko/blob/master/docs/dosing-pump.md).
