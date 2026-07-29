---
title: Erster Start & WiFi-Setup
description: Verbinde einen frisch geflashten Gekko-Controller ueber seinen Setup-Access-Point mit deinem WiFi-Netz.
sidebar:
  order: 4
---

Gekko wird mit **keinen hartkodierten WiFi-Zugangsdaten** ausgeliefert. Beim
ersten Start oeffnet das Geraet seinen eigenen Setup-Access-Point und du
konfigurierst dein Netzwerk im Portal.

## Ueber den Setup-Access-Point verbinden

1. Schalte das frisch geflashte Board ein. Nach wenigen Sekunden startet ein
   offener WiFi-Access-Point namens **`gekko-<suffix>`**, wobei das Suffix von
   der MAC-Adresse des Boards stammt - zwei Controller nebeneinander stoeren
   sich also nicht.
2. Verbinde dich mit diesem Access-Point von einem Telefon oder Laptop aus. Auf
   den meisten Systemen erscheint eine Captive-Portal-Abfrage; falls nicht,
   oeffne das Portal direkt per IP - `http://192.168.4.1/` (die Standard-ESP32-
   AP-Adresse).
3. Oeffne im Portal die **WiFi**-Seite. Das Geraet scannt nach Netzwerken in
   der Umgebung und zeigt eine Liste an.
4. Waehle dein Netzwerk, gib das Passwort ein und speichere.
5. Das Geraet verbindet sich als Station mit deinem Netz. Der Setup-AP wird
   von der WiFi-State-Machine verwaltet - er bleibt verfuegbar, bis die
   Station-Verbindung steht, damit dich ein Tippfehler nie aussperrt.

Nach erfolgreicher Verbindung oeffne das Portal unter der Adresse, die dein
Router dem Geraet zugewiesen hat (siehe die Client-Liste des Routers oder die
Seriell-Logzeile des Geraets). Ab dann laeuft das Portal ueber dein normales
Netz.

## Wenn die Verbindung fehlschlaegt

Gespeicherte Zugangsdaten fuer ein unerreichbares Netz machen das Geraet nicht
unbrauchbar: Verbindungsversuche der Station sind zeitgesteuert, und der
Setup-AP plus das Portal bleiben die ganze Zeit verfuegbar - verbinde dich
einfach erneut mit dem AP und korrigiere die Einstellungen.

## Alternative: BLE-Provisionierung

Die **Standard**-Firmware kann WiFi-Zugangsdaten auch ueber **Bluetooth LE**
mit einer Espressif-kompatiblen App fuer Android oder iOS empfangen. Schliesse
einen Schliesser zwischen GPIO 32 und GND an und halte ihn 3 Sekunden gedrueckt,
um den BLE-Konfigurationsmodus zu starten. Der Modus kann auch auf der
WiFi-Seite im Webportal oder per API gestartet werden. Die Sitzung hat einen
Timeout; gespeicherte Zugangsdaten werden erst nach erfolgreicher Uebertragung
neuer Werte geaendert.

Die **Without BLE**-Firmware enthaelt keinen BLE-Provisioning-Code und
reserviert GPIO 32 nicht. Setup-Access-Point und Webportal sind bei beiden
Firmware-Varianten verfuegbar.

## Naechstes

Sobald das Geraet im Netz ist, mach den
[Portal-Rundgang](/gekko/de/getting-started/portal-tour/) oder spring direkt zum
[ersten Geraet hinzufuegen](/gekko/de/getting-started/first-device/).
