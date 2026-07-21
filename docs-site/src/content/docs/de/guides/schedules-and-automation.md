---
title: Plaene & Automatisierung
description: Zeitplaene und bedingungsgetriebene Auto-Switches in Gekko.
sidebar:
  order: 2
---

Zwei Geraetetypen arbeiten zusammen, um Schalten zu automatisieren: Ein
**Schedule** haelt Zeitregeln und meldet, ob er gerade aktiv ist, und ein
**Auto-Switch** steuert einen echten Schalter aus dem logischen UND seiner
angehaengten Bedingungen.

## Schedule

Ein [Schedule-Geraet](/gekko/de/reference/devices/schedule/) haelt bis zu 4
Regeln. Jede Regel hat:

- eine **Wochentagsmaske** - an welchen Tagen sie gilt;
- ein **Zeitfenster** - Start- und Endminute des Tages (Minutenpraezision,
  keine Sekunden);
- einen **Modus**:
  - **Always on** - fuer das ganze Fenster aktiv;
  - **Interval** - teilt das Fenster in gleich grosse Abschnitte und ist fuer
    die ersten N Minuten jedes Abschnitts aktiv (fuer zirkulierende, nebelnde
    oder aehnliche periodische Aufgaben).

Der Schedule ist aktiv, wenn **irgendeine** aktivierte Regel passt. Der
Regel-Editor im Portal zeigt eine clientseitige Ein/Aus-Vorschau auf Basis der
Regeln gegen die Uhr deines Browsers - als Schaetzung markiert, weil das Geraet
mit seiner eigenen Uhr und Zeitzone rechnet.

:::note[Dem Geraet eine vernuenftige Uhr geben]
Schedules tun nichts, bis die Geraetuhr plausibel ist. Nutze NTP (Zeitzone auf
der **Time**-Seite setzen) oder fuege ein DS3231-RTC-Geraet hinzu, damit
Plaene auch bei Internetausfall und Reboots weiterlaufen.
:::

## Auto switch

Ein Auto-Switch umhuellt einen echten Schalter (GPIO oder Portexpander) und
steuert ihn aus bis zu **6 Bedingungs-Abhaengigkeiten** - Plaene, andere
Schalter oder andere Auto-Switches - jeweils optional **invertiert**. Alle
Bedingungen werden UND-verknuepft: Der Ausgang ist nur an, solange jede
Bedingung erfuellt ist. Ohne Bedingungen bleibt ein Auto-Switch im Auto-Modus
aus.

Seine Modi sind:

- **Off / On** - manuelles Override; Bedingungen werden ignoriert. Das
  Umschalten vom Dashboard setzt genau das.
- **Auto** - den Bedingungen folgen.
- **Paused** - fuer eine konfigurierte Dauer voruebergehend aus, dann
  automatisch wieder **Auto**. Pause ist nur aus dem Auto-Modus verfuegbar.
  Ein Reboot mitten in einer Pause setzt sie mit der korrekten Restzeit fort.

Beim Wechsel nach Auto (oder Paused) wird der Zielschalter immer zuerst
ausgeschaltet und dann an die Bedingungen uebergeben - damit ein frueherer
manueller "On"-Zustand nicht still weiterhängt.

Weil ein Auto-Switch selbst als Schalter und als Bedingung fungiert, kannst du
Automatisierungen ketten: Ein "Feeding mode"-Auto-Switch kann ueber seine
invertierten Bedingungs-Slots mehrere andere Auto-Switches zugleich sperren.

## Beispiel: Aquarienlicht mit Pause-Button

1. Erstelle einen **Schedule** "Light hours" mit Regel: jeden Tag, 09:00-21:00,
   immer an.
2. Erstelle einen **GPIO Switch** "Light relay" auf dem Pin, der dein Licht
   treibt.
3. Erstelle einen **Auto Switch** "Light": target = "Light relay", condition =
   "Light hours", mode = Auto.
4. Pinne "Light" auf das Dashboard. Er folgt jetzt dem Zeitplan; tippe ihn fuer
   ein manuelles Override an, nutze **pause** fuer Wartung und schalte danach
   wieder auf Auto.

## Verwandte Geraete

- **[Thermostat](/gekko/de/reference/devices/thermostat/)** - Hysterese-
  Temperaturregelung, die einen Schalter antreibt.
- **[Dosing pump](/gekko/de/reference/devices/dosing-pump/)** - geplante Dosierung
  mit Kalibrierung, Containerzaehlung und Dosiereintrag.
- **[Scheduled analog output](/gekko/de/reference/devices/analog-outputs/)** -
  eine taegliche Helligkeits-/Levelkurve fuer PWM-Ausgaenge, kombinierbar zu
  mehrkanaeligen Leuchten.
