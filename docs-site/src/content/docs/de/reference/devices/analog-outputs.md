---
title: Analoge Ausgaenge & Licht-Komposer
description: Dimmbarer PWM-Ausgaenge in Gekko - sanfte Uebergaenge, taegliche Helligkeitskurven und mehrkanaelige Leuchten wie ein fuenfkanaliges Aquariumlicht.
sidebar:
  order: 10
---

## Warum dimmbare Ausgaenge?

Ein Ein/Aus-Schalter reicht fuer einen Heizer - aber ein Licht sollte morgens
nicht von 0 auf 100 % springen. Ein gutes Aquarium- (oder Terrarium- oder
Gewaechshaus-) Licht:

- **faehrt langsam hoch und runter** - Sonnenaufgang und Sonnenuntergang, kein
  Lichtschalter; Fische schrecken auf harte Uebergaenge sichtbar auf, Korallen
  moegen sie auch nicht;
- **veraendert die Intensitaet ueber den Tag** - ein Mittagshoehepunkt,
  sanftere Morgen und Abende;
- **mischt mehrere Farbkanaele** - Reef-Leuchten haben typischerweise getrennte
  royal blue-, blue-, white-, violet- und moonlight-LED-Strenge, jede mit
  eigener Tageskurve.

Gekko modelliert das mit vier Geraetetypen, die wie Bausteine zusammenpassen.
Jeder Level ist ein Prozentwert (0-100 %) im Portal und in der API; die
Hardware-Seite ist ein ESP32-PWM-(LEDC-)Pin, der den Dimm-Eingang eines LED-
Treibers, ein MOSFET-Modul oder irgendeine andere PWM-gesteuerte Last ansteuert.

## Die Bausteine

| Typ | Was es macht |
| --- | --- |
| `analog_output` | Der Hardware-PWM-Kanal auf einem Pin |
| `fade_analog_output` | Glättet jede Aenderung zu einer sanften Rampe |
| `scheduled_analog_output` | Fuehrt sein Ziel entlang einer taeglichen Kurve |
| `analog_output_composer` | Gruppiert mehrere Kanaele zu einer Leuchte |

Ein Fade- oder Scheduled-Output hat genau eine `analog_output`-Rollen-
Abhaengigkeit und *stellt dieselbe Rolle selbst bereit*, also lassen sie sich
stapeln:

![Decorator-Kette: Scheduled-Output berechnet den Level, Fade glättet ihn, Analogausgang schreibt PWM](../../../../../assets/diagrams/analog-chain.svg)

- **Fade** - `maxStep` (Prozent pro Schritt) und `stepIntervalMs` bestimmen die
  Rampengeschwindigkeit; der Standard von etwa 1 % pro 200 ms macht jede
  Aenderung, auch einen manuellen Slider-Move, zu einem sanften Uebergang.
- **Scheduled** - bis zu 10 `(time, level)`-Punkte pro Tag, mit Interpolation
  zwischen den Punkten. Modi: **Off**, **Manual** (ein fixer Wert),
  **Scheduled** (der Kurve folgen). Ohne gueltige Uhr faellt der Ausgang auf
  Null statt einen alten Wert zu halten.

Das Register erzwingt, dass jeder Ausgang **hoechstens einen Controller** hat -
du kannst nicht versehentlich zwei Plaene auf denselben Kanal verdrahten.

## Beispiel: ein fuenfkanaliges Aquariumlicht

Das Ziel - ein Tag wie dieser:

![Taegliche Kurven von fuenf Kanaelen: Blues fahren frueh hoch und bleiben laenger an, Weiss peakt mittags, Violet setzt Akzente, Moonlight leuchtet nachts](../../../../../assets/diagrams/aquarium-light-day.svg)

Blau kommt zuerst hoch und faellt zuletzt ab (Korallen photosynthetisieren vor
allem im Blauen), warmes Weiss fuellt den Mittag, Violet sorgt fuer Fluoreszenz
und ein schwacher Moonlight-Kanal leuchtet nachts. So baust du das:

1. Erstelle fuenf **`analog_output`**-Geraete, eines pro LED-Treiber-Pin:
   "Royal blue LEDC", "Blue LEDC", "White LEDC", "Violet LEDC", "Moonlight
   LEDC".
2. Wickle jedes in ein **`fade_analog_output`** ("Royal blue fade" -> Ziel
   "Royal blue LEDC", ...) damit Kanaelwechsel nie springen.
3. Wickle jeden Fade in ein **`scheduled_analog_output`** ("Royal blue
   schedule" -> Ziel "Royal blue fade", ...) und zeichne die taegliche Kurve
   dieses Kanals.
4. Erstelle einen **`analog_output_composer`** "Aquarium light" und fuege die
   fuenf Scheduled-Outputs als Kanaele hinzu.

![Aquarium-Light-Komposer im Portal](../../../../../assets/screenshots/device-analog-composer.png)

Der Komposer verhält sich jetzt wie *das* Licht:

- **Ein Modus fuer die ganze Leuchte** - das Umschalten des Komposers zwischen
  Off / Manual / Scheduled pusht diesen Modus an alle Kanaele und haelt sie
  synchron, wenn einer auseinanderlaeuft. Off setzt alles auf Null.
- **Ein Editor** - alle Kanalkurven auf einem Graphen, direkt per Drag
  bearbeitet (Rechtsklick zum Einfuegen/Loeschen, optionales 15-Min/5 %-Snapping,
  Sonnenaufgang/Sonnenuntergang als Unterlage); der Manual-Modus zeigt einen
  Slider pro Kanal.
- **Eine Dashboard-Karte** - pinne den Komposer fuer eine kompakte Multi-Kanal-
  Vorschau.

Der Komposer ist nur noetig, wenn mehrere Kanaele als eine Leuchte wirken
sollen - ein Ein-Kanal-Licht ist einfach Schritt 1-3 mit nur einer Kette. Lass
die Fade-Ebene weg, wenn dir Rampen egal sind.

Eine vollständige Anleitung für Anschluss, Einrichtung und Prüfung bietet das
[Projekt Mehrkanal-Leuchte](/gekko/de/projects/multichannel-light/).

## Laufzeit & Steuerung

Alle vier Typen melden ihren Live-Level (Fades melden ausserdem das Ziel und
ob sie noch uebergehen). Ein Dashboard-Slider oder der `setOutput`-Befehl
steuert einen Kanal direkt; Moduswechsel laufen ueber `setMode`. Auf
[MQTT-Builds](/gekko/de/guides/mqtt-home-assistant/) sind Kanaele in Home
Assistant auffindbar. Interna:
[`docs/analog-output.md`](https://github.com/yoreek/gekko/blob/master/docs/analog-output.md).
