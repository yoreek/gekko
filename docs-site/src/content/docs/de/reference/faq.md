---
title: FAQ & Fehlerbehebung
description: Haeufige Probleme beim Flashen, Provisioning und Betrieb eines Gekko-Controllers - und ihre Loesungen.
sidebar:
  order: 3
---

## Flashen

### Der Webinstaller sagt, mein Browser wird nicht unterstuetzt

Web Serial gibt es nur in Chromium-Browsern - nutze **Chrome, Edge oder Opera
auf dem Desktop**. Firefox, Safari und alle Mobilbrowser koennen nicht flashen.
Alternativ nutze die [esptool-Skripte](/gekko/de/getting-started/flashing/), die
ueberall funktionieren.

### Der Installer zeigt den seriellen Port meines Boards nicht an

- Nutze ein USB-**Daten**kabel - viele beigelegte Kabel sind nur Ladekabel.
- Installiere den CP210x- oder CH340-Treiber, den der USB-Seriell-Chip deines
  Boards braucht.
- Unter Linux: fuege dich zur seriellen Gruppe hinzu (`sudo usermod -a -G
  dialout $USER`, dann neu anmelden) und beachte, dass manche Linux-/Chrome-/
  USB-Chip-Kombinationen ueber Web Serial bekanntlich zickig sind - der
  esptool-Pfad ist der verlaessliche Fallback.
- Schließe alles andere, was den Port belegen koennte (Seriell-Monitore, IDEs).

## Erster Start & WiFi

### Der `gekko-…`-Setup-Access-Point erscheint nie

- Gib dem Board nach dem Einschalten etwa 10 Sekunden.
- Wenn das Geraet schon einmal geflasht war und alte Zugangsdaten gespeichert
  hat, wechselt es direkt in den Station-Modus - schau stattdessen in die
  Client-Liste deines Routers.
- Reflash mit Erase-Option (im Webinstaller gibt es "erase device"; mit
  esptool zuerst `esptool erase_flash`), um wieder einen sauberen Erststart zu
  bekommen.

### Ich bin mit dem Setup-AP verbunden, aber es oeffnet sich kein Portal

Nicht jedes Betriebssystem oeffnet das Captive Portal automatisch. Oeffne
`http://192.168.4.1/` selbst im Browser.

### Ich habe falsche WiFi-Zugangsdaten gespeichert

Nichts geht verloren: Verbindungsversuche sind zeitgesteuert und der Setup-AP
bleibt parallel verfuegbar. Verbinde dich erneut mit dem `gekko-…`-AP und
korrigiere die Einstellungen auf der WiFi-Seite.

## Portal & Geraete

### Das Portal laedt, aber ein Geraet zeigt `dependency_blocked`

Eine seiner Abhaengigkeiten ist deaktiviert, geloescht oder fehlerhaft - z. B.
eine DS18B20, deren 1-Wire-Bus-Geraet deaktiviert ist. Repariere zuerst das
Eltern-Geraet; das Kind erholt sich von selbst.

### Meine DS18B20 erscheint nicht im Bus-Scan

Pruefe den ~4,7-kΩ-Pull-up zwischen Data und 3,3 V sowie die Verdrahtung. Eine
gesunde Sonde scannt mit Family Code `28` und einer 16-stelligen Adresse
ohne CRC-Flag.

### Plaene schalten nie etwas ein

Schedules brauchen eine plausible Uhr. Setze die Zeitzone und NTP auf der
**Time**-Seite oder fuege eine DS3231-RTC hinzu. Denk auch daran: Ein
Auto-Switch muss im **Auto**-Modus sein - ein manuelles Off/On-Override
ignoriert Bedingungen, und ein Auto-Switch ohne Bedingungen bleibt absichtlich
aus.

### Wo ist die OTA-/MQTT-Seite hin?

Diese Seiten erscheinen nur auf Firmware-Builds, die mit dem jeweiligen
Feature kompiliert wurden - siehe [OTA updates](/gekko/de/guides/ota-updates/)
und [MQTT & Home Assistant](/gekko/de/guides/mqtt-home-assistant/).

## Wiederherstellung

### Factory reset

Reflash mit Voll-Delete (Erase-Option des Webinstallers oder `esptool
erase_flash` + Reflash). Das loescht WiFi-Zugangsdaten und das ganze
Geraeteregister - exportiere vorher ein [Backup](/gekko/de/guides/backup-restore/),
wenn du das Setup spaeter wiederherstellen moechtest.

### Welche Firmware-Version laeuft gerade?

`GET /api/system/version`, die System-Seite im Portal oder die Zeile
`Gekko booting version=…` im seriellen Boot-Log.
