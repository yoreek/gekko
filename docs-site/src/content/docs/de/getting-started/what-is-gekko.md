---
title: Was ist Gekko?
description: Eine Einfuehrung in Gekko - ein modularer ESP32-Geratecontroller mit integriertem Webportal.
sidebar:
  order: 1
---

Gekko ist Firmware fuer den ESP32 plus ein Webportal, das direkt aus dem
Flash des Geraets ausgeliefert wird. Zusammen kannst du damit deinen eigenen
Controller bauen - Aquarium, Terrarium, Gewaechshaus oder allgemeine
Hausautomation - auf Basis eines Katalogs von Geraetetypen, die komplett ueber
die UI verdrahtet und konfiguriert werden.

**Ein Firmware-Image, kein Projekt-Reload pro Aenderung.** Jeder unterstuetzte
Geraetetyp ist bereits eingebaut. Ein Relais, einen Temperatursensor, ein
Display oder eine Dosierpumpe hinzuzufuegen ist eine Portal-Aktion auf dem
laufenden Geraet, nie ein Recompile.

## Was du damit bauen kannst

- **Schalter und Ausgaenge** - GPIO-Relais, Schalter hinter PCF8574/PCF8575
  I2C-Portexpandern, PWM/Analog-Ausgaenge mit sanften Uebergaengen, Tageskurven
  fuer Helligkeit und mehrkanaelige Gruppen.
- **Sensoren** - DS18B20- und NTC-Temperatursensoren, HTU21/AHT10/DHT11 fuer
  Temperatur + Luftfeuchte und digitale Binaereingaenge.
- **Automatisierung** - minuezise Tagesplaene, bedingungsgetriebene Auto-Switches
  mit manuellem Override und Pause, Hysterese-Thermostate und Dosierpumpen mit
  Kalibrierung und Dosiereintrag.
- **Displays** - SSD1306-OLED, ST7735-TFT, LCD1602/LCD2004-Zeichenanzeigen und
  TM1637-Siebensegmentmodule mit gemeinsamem visuellem Layout-Designer.
- **Infrastruktur** - I2C-/SPI-/1-Wire-Busse, DS3231/DS1302-Echtzeituhren und
  ein Dashboard, das du aus Panels zusammensetzt.

Geraete deklarieren **Abhaengigkeiten** untereinander - ein Schalter auf einem
Portexpander, ein Sensor auf einem I2C-Bus, eine Pumpe, die von einem Plan
gesteuert wird - und das Registry-System validiert, erzwingt und speichert
diesen Graphen. Siehe
[Geraete und Abhaengigkeiten](/gekko/de/guides/devices-and-dependencies/) fuer das
Konzept.

## Was Gekko unterscheidet

**Kein Firmware-Image pro Setup.** Viele Controller-Firmwares verwandeln deine
Konfiguration in einen dedizierten Build - einen Sensor hinzufuegen bedeutet,
eine Config-Datei zu bearbeiten, neu zu kompilieren und erneut zu flashen.
Gekko liefert ein einziges Image mit allen unterstuetzten Geraetetypen;
Aenderungen sind immer eine Portal-Aktion auf dem laufenden Geraet, nie ein
Neubuild.

**Struktur statt Pin-Tabellen.** Laufzeitkonfiguration bedeutet oft nur eine
flache Liste von GPIO-Zuordnungen und Konsolenregeln. Gekko modelliert die
Hardware stattdessen so, wie sie wirklich verdrahtet ist: ein typisiertes
Registry mit deklarierten Abhaengigkeiten und eigener versionierter Konfiguration,
die bei Firmware-Upgrades automatisch migriert.

**Alles ist beobachtbar und skriptbar.** Live-Zustand kommt per WebSocket,
jeder Geraetetyp spricht dieselbe REST-API, auffaellige Ereignisse landen im
Journal, Displays erhalten einen visuellen Designer und das Dashboard besteht
aus Panels - nicht aus einem einzelnen Konsolenbildschirm.

**Home Assistant mit einem Schalter.** Auf MQTT-faehigen Builds ist das
Veroeffentlichen eines Geraets zu Home Assistant nur ein Schalter auf seiner
Seite - es erscheint dort als native Entity (switch, sensor, climate), die du
aus HA steuern kannst, waehrend alles lokal weiterlaeuft. Siehe
[MQTT & Home Assistant](/gekko/de/guides/mqtt-home-assistant/).

Der ehrliche Kompromiss: Der Geraetetyp-Katalog ist zur Compile-Zeit fixiert
und damit bewusst eine kleinere, strukturiertere Basis als ein Katalog fuer
wirklich jeden Sensor.

## Alles laeuft auf dem Geraet

- **Local-first** - das Portal wird vom ESP32 ueber WiFi ausgeliefert; keine
  Cloud, kein Konto, kein App Store.
- **Optionale Integrationen** - MQTT + Home-Assistant-Discovery und OTA-Updates
  existieren, sind aber standardmaessig aus.
- **WiFi-Provisionierung** - ein Setup-Access-Point (oder BLE, wenn aktiviert)
  bringt das Geraet ins Netzwerk, ohne hartkodierte Zugangsdaten.
- **Backup und Restore** - das komplette Geraetesetup exportiert als ein
  einzelnes, menschenlesbares Bundle.
- **Sieben Sprachen** - das Portal erkennt die Browsersprache automatisch und
  ist auf Englisch, Ukrainisch, Russisch, Deutsch, Spanisch, Franzoesisch und
  Italienisch verfuegbar.

## Naechste Schritte

1. [Hardware-Anforderungen pruefen](/gekko/de/getting-started/hardware/)
2. [Firmware flashen](/gekko/de/getting-started/flashing/) - aus dem Browser
   oder mit esptool/PlatformIO
3. [Geraet mit WiFi verbinden](/gekko/de/getting-started/first-boot-wifi/)
4. [Erstes Geraet hinzufuegen](/gekko/de/getting-started/first-device/)
