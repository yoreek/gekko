---
title: Pixel Strip (WS2812B)
description: Adressierbare WS2812B-RGB-Streifen in Gekko - ein Hardware-Backend plus Effekte fuer Vollfarbe und Alarm-Blinken, beide live und aus Home Assistant steuerbar.
sidebar:
  order: 13
---

## Die Bausteine

Ein adressierbarer Streifen ist mehr als ein An/Aus- oder dimmbarer Ausgang -
er ist ein Array von Farben. Gekko modelliert das mit einem Hardware-Backend
und Effekt-Geraeten, die es ansteuern - dasselbe Decorator-Muster wie bei den
[analogen Ausgaengen](/gekko/de/reference/devices/analog-outputs/):

| Typ | Was er macht |
| --- | --- |
| `pixel_strip` | Ein WS2812B-Datenpin - besitzt den Pixelpuffer und schreibt ihn zur Hardware |
| `pixel_effect_solid` | Fuellt den Ziel-Streifen mit einer statischen Farbe |
| `pixel_effect_alert` | Laesst den Ziel-Streifen blinken, solange seine Bedingungen erfuellt sind |

Jeder Effekt nimmt genau eine `pixel_strip`-Abhaengigkeit und haelt sie
**exklusiv** - zwei Effekte koennen nicht gleichzeitig an denselben Streifen
verdrahtet werden, sie kommen sich also nie in die Quere. Effekte lassen sich
noch nicht verketten (anders als Fade/Scheduled bei analogen Ausgaengen);
jeder Streifen faehrt genau einen Effekt gleichzeitig.

## `pixel_strip`

Das Hardware-Geraet. Konfiguration:

- **Pin** - der GPIO, der an die Datenleitung des Streifens angeschlossen ist.
- **Pixelanzahl** - wie viele LEDs auf dem Streifen sitzen (bis zu 300).
- **Start-Helligkeit** - die Helligkeit, die beim Booten angewendet wird, wenn
  kein gespeicherter Zustand zum Wiederherstellen vorliegt.
- **Vorherigen Zustand wiederherstellen** - beim Start mit der zuletzt
  gesetzten Live-Helligkeit starten statt immer mit dem konfigurierten
  Startwert.

Helligkeit und An/Aus sind **Live-Zustand**, nicht Teil der gespeicherten
Konfiguration - den Dashboard-Regler zu ziehen oder das Geraet auszuschalten
markiert die Konfiguration nie als geaendert und fragt nie nach einem
Speichern-Dialog. Ausschalten zeigt an der Hardware immer Schwarz; wieder
einschalten stellt die zuletzt gesetzte Helligkeit wieder her, sodass du nie
einen Wert neu eingeben musst.

## `pixel_effect_solid`

Fuellt seinen Ziel-Streifen mit einer Farbe und haelt sie - die einfachste Art,
einen Streifen in nur einem Farbton leuchten zu lassen (ein Mondlicht-Kanal,
ein Akzentlicht, ein statisches Riff-Weiss).

- Der **Farbwaehler** setzt die Live-Farbe direkt aus dem Widget; der
  Farbwaehler im Konfigurationsformular setzt nur die **Startfarbe**, die
  beim Booten angewendet wird.
- **Vorherigen Zustand wiederherstellen** funktioniert genau wie bei
  `pixel_strip`: entweder die zuletzt gesetzte Live-Farbe wiederherstellen
  oder immer mit der Startfarbe beginnen.
- Dasselbe explizite An/Aus-Gate wie `pixel_strip` - Aus zeigt an der
  Hardware immer Schwarz, unabhaengig von der konfigurierten Farbe und
  unabhaengig davon, welche Farbe gerade gespeichert ist.

## `pixel_effect_alert`

Laesst seinen Ziel-Streifen zwischen einer konfigurierten **Farbe** und
Schwarz mit einem konfigurierten **Blink-Intervall** blinken, solange eine
begrenzte Liste von bis zu 4 `Condition`-Geraeten (ein Zeitplan, ein
Schalter, ein Auto-Switch, ...) alle erfuellt sind - derselbe
UND-Bedingungs-Mechanismus, den auch
[`auto_switch`](/gekko/de/guides/schedules-and-automation/) nutzt. Eine leere
Bedingungsliste ist nie erfuellt, sodass ein falsch konfigurierter Alarm nicht
versehentlich blinken kann. Anders als bei `pixel_strip`/`pixel_effect_solid`
sind Farbe und Blink-Intervall hier normale, persistierte Konfiguration - Farbe
und Takt eines Alarms beschreiben, was der Alarm *bedeutet*, keinen Wert, den
man live anpassen wuerde.

Typischer Einsatz: einen Ueberlauf-Schwimmerschalter oder die abgeleitete
Bedingung eines Leck-`binary_sensor` mit einem roten Alarm-Streifen neben dem
Becken verdrahten.

## Laufzeit & Steuerung

`pixel_strip` meldet seine Live-Helligkeit und Pixelanzahl;
`pixel_effect_alert` meldet, ob seine Bedingungen gerade erfuellt sind. Ein
Dashboard-Regler oder Farbwaehler steuert Helligkeit/Farbe direkt.

Auf [MQTT-Builds](/gekko/de/guides/mqtt-home-assistant/) sind alle drei Typen
in Home Assistant sichtbar: `pixel_strip` und `pixel_effect_solid`
veroeffentlichen sich jeweils als `light` (nur Helligkeit bzw. nur RGB), und
`pixel_effect_alert` veroeffentlicht sich als `binary_sensor`. Internes:
[`docs/pixel-strip.md`](https://github.com/yoreek/gekko/blob/master/docs/pixel-strip.md).
